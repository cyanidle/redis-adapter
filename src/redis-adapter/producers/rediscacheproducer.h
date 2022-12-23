#ifndef REDISCACHEPRODUCER_H
#define REDISCACHEPRODUCER_H

#include <QObject>
#include "redis-adapter/connectors/redisconnector.h"
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "JsonFormatters"

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
    int writeKeys(const Formatters::JsonDict &json, const Radapter::WorkerMsg &msg);
    int writeIndex(const Formatters::JsonDict &json, const QString &indexKey, const Radapter::WorkerMsg &msg);
    // Replies
    void writeKeysDone(const Radapter::WorkerMsg &msg);
    void writeIndexDone(const Radapter::WorkerMsg &msg);

    static void msetCallback(redisAsyncContext *context, void *replyPtr, void *args, const Radapter::WorkerMsg &msg);
    static void indexCallback(redisAsyncContext *context, void *replyPtr, void *args, const Radapter::WorkerMsg &msg);

    QString m_indexKey;
};

#endif // REDISCACHEPRODUCER_H
