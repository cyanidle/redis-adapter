#ifndef REDISCACHECONSUMER_H
#define REDISCACHECONSUMER_H

#include "redis-adapter/connectors/redisconnector.h"

namespace Redis{
class CacheConsumer;
struct CacheContext
{
public:
    using CtxHandle = void*;
    enum Type {
        Error = 0,
        IndexRead,
        SimpleRead,
        PackRead
    };
    CacheContext() = default;
    CacheContext(const Radapter::WorkerMsg &msgToReply, CacheConsumer *parent);
    Radapter::WorkerMsg &msg() {return m_msg;}
    const Radapter::WorkerMsg &msg() const {return m_msg;}
    void addNestedObjectResult(Radapter::Command &nextStep, const QString &key, const QString &value);
    void fail(const QString &reason = {});
    void reply(Radapter::Reply &reply);
    void reply(Radapter::Reply &&reply) {this->reply(reply);}
    bool isDone() const {return m_isDone;}
    Radapter::CommandPack *packFromMsg();
    void executeNextForIndex();
    bool operator==(const CacheContext &other) const {
        return msg().id() == other.msg().id();
    }
private:
    void replyIndex(Radapter::Reply &reply);
    void replySimple(Radapter::Reply &reply);
    void replyPack(Radapter::Reply &reply);

    CacheConsumer *m_parent{};
    Radapter::WorkerMsg m_msg{};
    Type m_type{};
    quint16 m_executedCount{0};
    quint8 m_inReply{};
    bool m_isDone{false};
    Radapter::CommandPack m_pack{};

    Radapter::ReplyPack m_replyPack{};
};

class RADAPTER_SHARED_SRC CacheConsumer : public Connector
{
    Q_OBJECT
    using CtxHandle = void*;
public:
    explicit CacheConsumer(const Settings::RedisCacheConsumer &config, QThread *thread);
public slots:
    void onCommand(const Radapter::WorkerMsg &msg) override;
private:
    CtxHandle getHandle(const CacheContext &ctx);
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

    friend CacheContext;
    QString m_indexKey;
    Radapter::ContextManager<CtxHandle, CacheContext> m_manager{};
};
}

#endif // REDISCACHECONSUMER_H
