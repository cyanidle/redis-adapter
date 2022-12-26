#ifndef REDISCACHEPRODUCER_H
#define REDISCACHEPRODUCER_H

#include <QObject>
#include "redis-adapter/connectors/redisconnector.h"
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "jsondict/jsondict.h"

namespace Redis {
class RADAPTER_SHARED_SRC CacheProducer;
}

class Redis::CacheProducer : public Connector
{
    Q_OBJECT
public:
    explicit CacheProducer(const QString &host,
                           const quint16 port,
                           const quint16 dbIndex,
                           const QString &indexKey,
                           const Radapter::WorkerSettings &settings, QThread *thread);
signals:

public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;

private:
    // Commands
    int writeKeys(const JsonDict &json, const Radapter::WorkerMsg &msg);
    int writeIndex(const JsonDict &json, const QString &indexKey, const Radapter::WorkerMsg &msg);
    // Replies
    void writeKeysDone(const Radapter::WorkerMsg &msg);
    void writeIndexDone(const Radapter::WorkerMsg &msg);

    void msetCallback(redisAsyncContext *context, redisReply *replyPtr, void *msgId);
    void indexCallback(redisAsyncContext *context, redisReply *replyPtr, void *msgId);

    QString m_indexKey;
};

#endif // REDISCACHEPRODUCER_H
