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
                           const Radapter::WorkerSettings &settings, QThread *thread);
public slots:
    void onCommand(const Radapter::WorkerMsg &msg) override;

private:
    //Commands
    void requestIndex(const QString &indexKey, const Radapter::WorkerMsg &msg);
    void requestKeys(const Formatters::List &keys, const Radapter::WorkerMsg &msg);
    //Replies
    void finishIndex(const Formatters::List &json, const Radapter::WorkerMsg &msg);
    void finishKeys(const Formatters::Dict &json, const Radapter::WorkerMsg &msg);

    static void readIndexCallback(redisAsyncContext *context, void *replyPtr, void *sender, const Radapter::WorkerMsg &msg);
    static void readKeysCallback(redisAsyncContext *context, void *replyPtr, void *sender, const Radapter::WorkerMsg &msg);


    Formatters::Dict mergeWithKeys(const Formatters::List &entries);
    QQueue<Formatters::List> m_requestedKeysBuffer;

    QString m_indexKey;
};

#endif // REDISCACHECONSUMER_H
