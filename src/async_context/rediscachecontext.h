#ifndef REDIS_CACHECONTEXT_H
#define REDIS_CACHECONTEXT_H

#include "contextmanager.h"
#include "broker/commands/basiccommands.h"
#include "broker/worker/workermsg.h"

namespace Redis {
class CacheProducer;
class CacheConsumer;
class Connector;

struct CacheContext  : Radapter::ContextBase {
    CacheContext(Connector *parent);
    void fail(const QString &reason = {});
    virtual void reply(Radapter::Reply &reply) = 0;
    void reply(Radapter::Reply &&reply);
    Radapter::WorkerMsg prepareReply(const Radapter::WorkerMsg &msg, Radapter::Reply *reply);
    Radapter::WorkerMsg prepareMsg(const JsonDict &json);
    void handleCommand(Radapter::Command *command, Handle handle);
    void sendMsg(const Radapter::WorkerMsg &msg);
    virtual ~CacheContext() = default;
protected:
    CacheProducer *m_prod;
    CacheConsumer *m_cons;
};

struct CacheContextWithReply : CacheContext
{
public:
    using CtxHandle = void*;
    CacheContextWithReply(const Radapter::WorkerMsg &msgToReply, Connector *parent);
    Radapter::WorkerMsg &msg();
    const Radapter::WorkerMsg &msg() const;
    bool operator==(const CacheContextWithReply &other) const {
        return msg().id() == other.msg().id();
    }
private:
    void replyObject(Radapter::Reply &reply);
    void replyPack(Radapter::Reply &reply);

    Radapter::WorkerMsg m_msg{};
};

struct SimpleContext : CacheContextWithReply {
    using CacheContextWithReply::CacheContextWithReply;
    void reply(Radapter::Reply &reply) override;
};

struct PackContext : CacheContextWithReply {
    PackContext(const Radapter::WorkerMsg &msgToReply, Connector *parent);
    void reply(Radapter::Reply &reply) override;
protected:
    Radapter::CommandPack *packInMsg();
private:
    Radapter::ReplyPack m_replyPack{};
};

struct SimpleMsgContext : CacheContext {
    SimpleMsgContext(Connector *parent) : CacheContext(parent) {}
    void reply(Radapter::Reply &reply) override;
};


struct NoReplyContext : CacheContext {
    NoReplyContext(Connector *parent) : CacheContext(parent) {}
    void reply(Radapter::Reply &reply) override;
};

} // namespace Redis

#endif // REDIS_CACHECONTEXT_H
