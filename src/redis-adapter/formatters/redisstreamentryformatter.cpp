#include "redisstreamentryformatter.h"
#include "timeformatter.h"

RedisStreamEntryFormatter::RedisStreamEntryFormatter(const JsonDict &streamReply) :
    m_streamEntry(streamReply)
{

}

RedisStreamEntryFormatter::RedisStreamEntryFormatter(JsonDict &&streamReply) :
    m_streamEntry(std::move(streamReply))
{

}

RedisStreamEntryFormatter::RedisStreamEntryFormatter(redisReply *streamReply)
    : m_streamEntry{}
{
    auto entryId = QString(streamReply->element[0]->str);
    if (entryId.isEmpty()) {
        return;
    }
    auto itemData = streamReply->element[1];
    if (itemData->elements == 0) {
        return;
    }

    auto fields = JsonDict{};
    for (quint16 i = 0; i < itemData->elements; i += 2) {
        auto fieldKey = QString(itemData->element[i]->str);
        auto fieldValue = QString(itemData->element[i + 1]->str);
        if (!fieldValue.isEmpty()) {
            fields.insert(fieldKey.split(":"), fieldValue);
        }
    }
    if (!fields.isEmpty()) {
        auto jsonEntry = JsonDict{};
        jsonEntry.insert(entryId, fields);
        m_streamEntry = jsonEntry;
    }
}

QString RedisStreamEntryFormatter::entryId() const
{
    auto entryId = m_streamEntry.firstKey();
    return entryId;
}

QDateTime RedisStreamEntryFormatter::eventTime() const
{
    auto timestamp = entryId().split("-").first().toLongLong();
    auto eventTime = TimeFormatter(timestamp).dateTime();
    return eventTime;
}

QStringList RedisStreamEntryFormatter::entryKeys() const
{
    auto keysList = eventDataDict().keysDeep();
    return QStringList(keysList);
}

JsonDict RedisStreamEntryFormatter::eventDataDict() const
{
    auto keyValueDict = JsonDict(m_streamEntry.first());
    return keyValueDict;
}

JsonDict RedisStreamEntryFormatter::toJson() const
{
    return m_streamEntry;
}
