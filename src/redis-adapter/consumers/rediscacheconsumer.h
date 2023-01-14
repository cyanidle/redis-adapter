#ifndef REDISCACHECONSUMER_H
#define REDISCACHECONSUMER_H

#include <QObject>
#include <QFuture>
#include "radapter-broker/future.h"
#include "redis-adapter/connectors/redisconnector.h"
#include "jsondict/jsondict.hpp"
#include <QQueue>

namespace Redis{

class RADAPTER_SHARED_SRC CacheConsumer : public Connector
{
    Q_OBJECT
public:
    explicit CacheConsumer(const QString &host,
                           const quint16 port,
                           const quint16 dbIndex,
                           const QString &indexKey,
                           const Radapter::WorkerSettings &settings,
                            QThread *thread);
    Future<QVariantList> requestIndex(const QString &indexKey);
    Future<JsonDict> requestKeys(const QStringList &keys);

public slots:
    void onCommand(const Radapter::WorkerMsg &msg) override;

private:
    //Commands
    void requestIndex(const QString &indexKey, const Radapter::WorkerMsg &msg);
    void requestKeys(const QStringList &keys, const Radapter::WorkerMsg &msg);
    void requestIndex(const QString &indexKey, void *data);
    void requestKeys(const QStringList &keys, void *data);
    //Replies
    void finishIndex(const QVariantList &json, const Radapter::WorkerMsg &msg);
    void finishKeys(const JsonDict &json, const Radapter::WorkerMsg &msg);

    void readIndexCallback(redisReply *replyPtr, void *msgId);
    void readKeysCallback(redisReply *replyPtr, void *msgId);


    JsonDict mergeWithKeys(const QVariantList &entries);
    QQueue<QStringList> m_requestedKeysBuffer;

    QString m_indexKey;
};
}

#endif // REDISCACHECONSUMER_H
