#ifndef REDISCACHECONSUMER_H
#define REDISCACHECONSUMER_H

#include <QObject>
#include "redis-adapter/connectors/redisconnector.h"
#include "jsondict/jsondict.hpp"
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
    void requestKeys(const QStringList &keys, const Radapter::WorkerMsg &msg);
    //Replies
    void finishIndex(const QVariantList &json, const Radapter::WorkerMsg &msg);
    void finishKeys(const JsonDict &json, const Radapter::WorkerMsg &msg);

    void readIndexCallback(redisAsyncContext *context, redisReply *replyPtr, void *msgId);
    void readKeysCallback(redisAsyncContext *context, redisReply *replyPtr, void *msgId);


    JsonDict mergeWithKeys(const QVariantList &entries);
    QQueue<QStringList> m_requestedKeysBuffer;

    QString m_indexKey;
};

#endif // REDISCACHECONSUMER_H
