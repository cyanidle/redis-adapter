#include "redisstreamqueries.h"
#include "jsondict/jsondict.hpp"
#include <QStringList>

QString Redis::addToStream(const QString &stream, const JsonDict &data, quint32 size)
{
    QString fields;
    for (const auto &iter : data) {
        fields.append(iter.key().join(':') + " " + iter.value().toString());
    }
    return QStringLiteral("XADD %1 MAXLEN ~ %2 * %3").arg(stream).arg(size).arg(fields);
}

QString Redis::trimStream(const QString &stream, quint32 maxLen)
{
    return QStringLiteral("XTRIM %1 MAXLEN ~ %2").arg(stream).arg(maxLen);
}

QString Redis::readStream(const QString &stream, const qint32 count, const qint32 blockTimeout, const QString &lastId)
{
    return QStringLiteral("XREAD COUNT %1 BLOCK %2 STREAMS %3 %4").arg(count).arg(blockTimeout).arg(stream, lastId);
}

QString Redis::readGroup(const QString &stream, const QString &groupName, const QString &consumerName, qint32 blockTimeout, const QString &id)
{
    return QStringLiteral("XREADGROUP GROUP %1 %2 BLOCK %3 STREAMS %4 %5").arg(groupName, consumerName).arg(blockTimeout).arg(stream, id);
}

QString Redis::ackEntries(const QString &streamKey, const QString &groupName, const QStringList &idList)
{
    return QString("XACK %1 %2 %3").arg(streamKey, groupName, idList.join(" "));
}

QString Redis::createGroup(const QString &streamKey, const QString &groupName, const QString &startId)
{
    return QStringLiteral("XGROUP CREATE %1 %2 %3 MKSTREAM").arg(streamKey, groupName, startId);
}

