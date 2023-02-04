#ifndef REDISCACHECONSUMER_H
#define REDISCACHECONSUMER_H

#include "redis-adapter/connectors/redisconnector.h"
#include "redis-adapter/contexts/rediscachecontext.h"

namespace Redis{


class RADAPTER_SHARED_SRC CacheConsumer : public Connector
{
    Q_OBJECT
    using CtxHandle = Radapter::ContextBase::Handle;
public:
    explicit CacheConsumer(const Settings::RedisCacheConsumer &config, QThread *thread);
public slots:
    void onCommand(const Radapter::WorkerMsg &msg) override;
private:
    CacheContext &getCtx(CtxHandle handle);
    void handleCommand(const Radapter::Command* command, CtxHandle handle);

    void requestMultiple(const Radapter::CommandPack *pack, CtxHandle handle);

    void requestSet(const QString &setKey, CtxHandle handle);
    void readSetCallback(redisReply *replyPtr, CtxHandle handle);

    void requestObject(const QString &objectKey, CtxHandle handle);
    void readObjectCallback(redisReply *replyPtr, CtxHandle handle);

    void requestKeys(const QStringList &keys, CtxHandle handle);
    void readKeysCallback(redisReply *replyPtr, CtxHandle handle);

    void requestHash(const QString &hash, CtxHandle handle);
    void readHashCallback(redisReply *replyPtr, CtxHandle handle);

    QString m_indexKey;
    Radapter::ContextManager<CacheContext> m_manager{};
    void requestKey(const QString &key, CtxHandle handle);
    void readKeyCallback(redisReply *replyPtr, CtxHandle handle);

    friend CacheContext;
    friend SimpleContext;
    friend ObjectContext;
    friend PackContext;
    static QRegExp m_firstIsIntChecker;
    static QRegExp m_intChecker;
    JsonDict parseNestedArrays(const JsonDict &target);
};
}

#endif // REDISCACHECONSUMER_H
