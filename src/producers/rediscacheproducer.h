#ifndef REDISCACHEPRODUCER_H
#define REDISCACHEPRODUCER_H

#include <QObject>
#include "connectors/redisconnector.h"
#include "jsondict/jsondict.hpp"
#include "async_context/rediscachecontext.h"

namespace Redis {
class RADAPTER_SHARED_SRC CacheProducer;
}

class Redis::CacheProducer : public Connector
{
    Q_OBJECT
public:
    using Handle = Radapter::ContextBase::Handle;
    explicit CacheProducer(const Settings::RedisCacheProducer &config, QThread *thread);
signals:

public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
    void onCommand(const Radapter::WorkerMsg &msg) override;
private:
    void handleCommand(Radapter::Command *command, Handle handle);

    int writeKeys(const JsonDict &json);
    int writeObject(const JsonDict &json, const QString &indexKey);
    void msetCallback(redisReply *replyPtr);
    void indexCallback(redisReply *replyPtr);

    QString m_objectKey;
    friend CacheContext;
    Radapter::ContextManager<CacheContext> m_manager;
};

#endif // REDISCACHEPRODUCER_H
