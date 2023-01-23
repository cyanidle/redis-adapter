#ifndef REDISCACHECONSUMER_H
#define REDISCACHECONSUMER_H

#include <QObject>
#include <QFuture>
#include "future/future.h"
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
    void requestIndex(const QString &indexKey, FutureSetter<QVariantList> setter);
    void readIndexCallback(redisReply *replyPtr, FutureSetter<QVariantList> *setter);

    void requestKeys(const QStringList &keys, FutureSetter<JsonDict> setter);
    void readKeysCallback(redisReply *replyPtr, FutureSetter<JsonDict> *setter);


    JsonDict mergeWithKeys(const QVariantList &entries);
    QString m_indexKey;
    QQueue<QStringList> m_requestedKeysBuffer{};
    QList<FutureSetter<JsonDict>> m_keysSetters{};
    QList<FutureSetter<QVariantList>> m_indexSetters{};
};
}

#endif // REDISCACHECONSUMER_H
