#ifndef REDISCACHEPRODUCER_H
#define REDISCACHEPRODUCER_H

#include <QObject>
#include "redis-adapter/connectors/redisconnector.h"
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "json-formatters/formatters/jsondict.h"

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
                           const Radapter::WorkerSettings &settings);
signals:

public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;

private:
    // Commands
    void writeKeys(const Formatters::JsonDict &json, int msgId);
    void writeIndex(const Formatters::JsonDict &json, const QString &indexKey, int msgId);
    // Replies
    void writeKeysDone(int msgId);
    void writeIndexDone(int msgId);

    static void msetCallback(redisAsyncContext *context, void *replyPtr, void *args);
    static void indexCallback(redisAsyncContext *context, void *replyPtr, void *args);

    QString m_indexKey;
};

#endif // REDISCACHEPRODUCER_H
