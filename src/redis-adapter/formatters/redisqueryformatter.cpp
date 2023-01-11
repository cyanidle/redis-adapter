#include "redisqueryformatter.h"
#include "redis-adapter/include/redismessagekeys.h"

RedisQueryFormatter::RedisQueryFormatter(const JsonDict &jsonDict, QObject *parent)
    : QObject(parent),
      m_json(jsonDict)
{
}

QString RedisQueryFormatter::toAddStreamCommand(const QString &streamKey, quint32 maxLen) const
{
    if (streamKey.isEmpty()) {
        return QString{};
    }
    auto command = QString("XADD %1 MAXLEN ~ %2 * ").arg(streamKey).arg(maxLen);
    command += toEntryFields();
    return command;
}

QString RedisQueryFormatter::toTrimCommand(const QString &streamKey, quint32 maxLen) const
{
    if (streamKey.isEmpty()) {
        return QString{};
    }
    auto command = QString("XTRIM %1 MAXLEN ~ %2").arg(streamKey).arg(maxLen);
    return command;
}

QString RedisQueryFormatter::toReadStreamCommand(const QString &streamKey, qint32 blockTimeout, const QString &lastId)
{
    if (streamKey.isEmpty()) {
        return QString{};
    }
    auto id = lastId.isEmpty() ? "$" : lastId;
    auto command = QString("XREAD BLOCK %1 STREAMS %2 %3").arg(blockTimeout).arg(streamKey, id);
    return command;
}

QString RedisQueryFormatter::toReadGroupCommand(const QString &streamKey, const QString &groupName, const QString &consumerName, qint32 blockTimeout, const QString &lastId)
{
    if (streamKey.isEmpty() || groupName.isEmpty() || consumerName.isEmpty()) {
        return QString{};
    }
    auto id = lastId.isEmpty() ? ">" : lastId;
    auto command = QString("XREADGROUP GROUP %1 %2 BLOCK %3 STREAMS %4 %5").arg(groupName, consumerName)
            .arg(blockTimeout).arg(streamKey, id);
    return command;
}

QString RedisQueryFormatter::toReadStreamAckCommand(const QString &streamKey, const QString &groupName, const QStringList &idList)
{
    if (streamKey.isEmpty() || groupName.isEmpty() || idList.isEmpty()) {
        return QString{};
    }
    auto command = QString("XACK %1 %2 ").arg(streamKey, groupName);
    command += idList.join(" ");
    return command;
}

QString RedisQueryFormatter::toCreateGroupCommand(const QString &streamKey, const QString &groupName, const QString &startId)
{
    if (streamKey.isEmpty() || groupName.isEmpty()) {
        return QString{};
    }
    auto id = startId.isEmpty() ? "$" : startId;
    auto command = QString("XGROUP CREATE %1 %2 %3 MKSTREAM").arg(streamKey, groupName, id);
    return command;
}

QString RedisQueryFormatter::toGetIndexCommand(const QString &indexKey)
{
    auto command = QString("SMEMBERS %1").arg(indexKey);
    return command;
}

QString RedisQueryFormatter::toUpdateIndexCommand(const QString &indexKey) const
{
    auto command = QString{};
    auto updatedKeys = flatten().keysDeep();
    if (!updatedKeys.isEmpty()) {
        command = QString("SADD %1 ").arg(indexKey);
        command += updatedKeys.join(" ");
    }
    return command;
}

QString RedisQueryFormatter::toMultipleGetCommand(const QStringList &keysList)
{
    if (keysList.isEmpty()) {
        return QString{};
    }

    auto command = QString("MGET ");
    command += toKeysFields(keysList);
    return command;
}

QString RedisQueryFormatter::toMultipleSetCommand() const
{
    auto command = QString("MSET ");
    command += toEntryFields();
    return command;
}

QString RedisQueryFormatter::toKeyEventsSubscribeCommand(const QStringList &eventTypes) const
{
    auto patternsList = QStringList{};
    for (auto &keyEventType : eventTypes) {
        auto pattern = QString("%1:%2").arg(REDIS_PATTERN_KEY_EVENT, keyEventType);
        patternsList.append(pattern);
    }
    auto command = toPatternSubscribeCommand(patternsList);
    return command;
}

QString RedisQueryFormatter::toPatternSubscribeCommand(const QStringList &patternList) const
{
    auto command = QString("PSUBSCRIBE ");
    command += toKeysFields(patternList);
    return command;
}

 JsonDict RedisQueryFormatter::flatten() const
{
    return m_json.flatten();
}

QString RedisQueryFormatter::toEntryFields() const
{
    auto flattenedJson = flatten();
    auto fieldsList = QStringList{};
    for (auto jsonItem = flattenedJson.begin();
         jsonItem != flattenedJson.end();
         jsonItem++)
    {
        fieldsList.append(jsonItem.key());
        fieldsList.append(jsonItem.value().toString());
    }
    return fieldsList.join(" ");
}

QString RedisQueryFormatter::toKeysFields(const QStringList &keysList)
{
    if (keysList.isEmpty()) {
        return QString{};
    }

    auto stringKeys = QVariant(keysList).toStringList();
    auto joinedKeys = stringKeys.join(" ");
    return joinedKeys;
}
