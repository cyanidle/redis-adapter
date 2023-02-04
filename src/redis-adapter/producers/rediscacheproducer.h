#ifndef REDISCACHEPRODUCER_H
#define REDISCACHEPRODUCER_H

#include <QObject>
#include "redis-adapter/connectors/redisconnector.h"
#include "jsondict/jsondict.hpp"

namespace Redis {
class RADAPTER_SHARED_SRC CacheProducer;
}

class Redis::CacheProducer : public Connector
{
    Q_OBJECT
public:
    explicit CacheProducer(const Settings::RedisCacheProducer &config, QThread *thread);
signals:

public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;

private:
    // Pipe
    int writeKeys(const JsonDict &json);
    int writeObject(const JsonDict &json, const QString &indexKey);
    void msetCallback(redisReply *replyPtr);
    void indexCallback(redisReply *replyPtr);

    QString m_indexKey;
};

#endif // REDISCACHEPRODUCER_H
