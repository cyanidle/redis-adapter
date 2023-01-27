#ifndef REDISCACHECONSUMER_H
#define REDISCACHECONSUMER_H

#include <QObject>
#include <QFuture>
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
public slots:
    void onCommand(const Radapter::WorkerMsg &msg) override;
private:
    void handleCommand(const Radapter::Command* command, const Radapter::WorkerMsg &msg);
    void requestMultiple(const QSet<Radapter::Command*> &commands, const Radapter::WorkerMsg &msg);

    void requestSet(const QString &setKey, const Radapter::WorkerMsg &msg);
    void readSetCallback(redisReply *replyPtr, Radapter::WorkerMsg *msg);

    void requestIndex(const QString &indexKey, const Radapter::WorkerMsg &msg);
    void readIndexCallback(redisReply *replyPtr, Radapter::WorkerMsg *msg);

    void requestKeys(const QStringList &keys, const Radapter::WorkerMsg &msg);
    void readKeysCallback(redisReply *replyPtr, Radapter::WorkerMsg *msg);

    void requestHash(const QString &hash, const Radapter::WorkerMsg &msg);
    void readHashCallback(redisReply *replyPtr, Radapter::WorkerMsg *msg);

    void failMsg(Radapter::WorkerMsg *msg, const QString &reason = "Not Given");
    static JsonDict mergeWithKeys(const QStringList &keys, const QStringList &entries);

    QString m_indexKey;
};
}

#endif // REDISCACHECONSUMER_H
