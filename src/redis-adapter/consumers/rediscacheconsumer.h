#ifndef REDISCACHECONSUMER_H
#define REDISCACHECONSUMER_H

#include <QObject>
#include "redis-adapter/connectors/redisconnector.h"
#include "JsonFormatters"
#include <QQueue>

namespace Redis{
class RADAPTER_SHARED_SRC CacheConsumer;
}

class Redis::CacheConsumer : public Connector
{
    Q_OBJECT
public:
    explicit CacheConsumer(const QString &host,
                           const quint16 port,
                           const quint16 dbIndex,
                           const QString &indexKey,
                           const Radapter::WorkerSettings &settings);
public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
    void onCommand(const Radapter::WorkerMsg &msg) override;

private:
    //Commands
    void requestIndex(const QString &indexKey, quint64 msgId);
    void requestKeys(const Formatters::List &keys, quint64 msgId);
    //Replies
    void finishIndex(const Formatters::List &json, quint64 msgId);
    void finishKeys(const Formatters::Dict &json, quint64 msgId);

    static void readIndexCallback(redisAsyncContext *context, void *replyPtr, void *args);
    static void readKeysCallback(redisAsyncContext *context, void *replyPtr, void *args);


    Formatters::Dict mergeWithKeys(const Formatters::List &entries);
    QQueue<Formatters::List> m_requestedKeysBuffer;

    QString m_indexKey;
};

#endif // REDISCACHECONSUMER_H
