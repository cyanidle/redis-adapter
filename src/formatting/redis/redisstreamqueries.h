#ifndef REDIS_STREAM_QUIERIES
#define REDIS_STREAM_QUIERIES

#include <QString>
class JsonDict;

namespace Redis {

QString addToStream(const QString &stream, const JsonDict &data, quint32 size = 0u);
QString trimStream(const QString &stream, quint32 maxLen);
QString readStream(const QString &stream, const qint32 count, const qint32 blockTimeout, const QString &lastId);
QString readGroup(const QString &stream, const QString &groupName, const QString &consumerName, qint32 blockTimeout, const QString &id);
QString ackEntries(const QString &streamKey, const QString &groupName, const QStringList &idList);
QString createGroup(const QString &streamKey, const QString &groupName, const QString &startId);

}

#endif
