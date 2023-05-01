#ifndef REDISCACHECONSUMER_H
#define REDISCACHECONSUMER_H

#include "connectors/redisconnector.h"
#include "async_context/rediscachecontext.h"

namespace Settings {
struct RedisCacheConsumer;
}

namespace Redis{
class RADAPTER_API CacheConsumer : public Connector
{
    Q_OBJECT
    struct Private;
    using CtxHandle = Radapter::ContextBase::Handle;
public:
    explicit CacheConsumer(const Settings::RedisCacheConsumer &config, QThread *thread);
    ~CacheConsumer() override;
public slots:
    void onCommand(const Radapter::WorkerMsg &msg) override;
private slots:
    void onDisconnect();
    void subscribe();
    void onEvent(redisReply *);
private:
    void onRun() override;
    CacheContext &getCtx(CtxHandle handle);
    void handleCommand(const Radapter::Command* command, CtxHandle handle);

    void requestSet(const QString &setKey, CtxHandle handle);
    void readSetCallback(redisReply *replyPtr, CtxHandle handle);

    void requestObjectSimple();
    void requestObject(const QString &objectKey, CtxHandle handle);
    void readObjectCallback(redisReply *replyPtr, CtxHandle handle);

    void requestKeys(const QStringList &keys, CtxHandle handle);
    void readKeysCallback(redisReply *reply, CtxHandle handle);

    void requestHash(const QString &hash, CtxHandle handle);
    void readHashCallback(redisReply *replyPtr, CtxHandle handle);
    void requestKey(const QString &key, CtxHandle handle);
    void readKeyCallback(redisReply *replyPtr, CtxHandle handle);

    JsonDict parseNestedArrays(const JsonDict &target);

    friend CacheContext;
    Private *d;
};
}

#endif // REDISCACHECONSUMER_H
