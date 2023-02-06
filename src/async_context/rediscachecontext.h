#ifndef REDIS_CACHECONTEXT_H
#define REDIS_CACHECONTEXT_H

#include "contextmanager.h"
#include "broker/commands/basiccommands.h"
#include "broker/worker/workermsg.h"

namespace Redis {

class Connector;
struct CacheContext : Radapter::ContextBase
{
public:
    using CtxHandle = void*;
    CacheContext(const Radapter::WorkerMsg &msgToReply, Connector *parent);
    Radapter::WorkerMsg &msg();
    const Radapter::WorkerMsg &msg() const;
    void fail(const QString &reason = {});
    virtual void reply(Radapter::Reply &reply) = 0;
    void reply(Radapter::Reply &&reply);
    Radapter::WorkerMsg prepareReply(const Radapter::WorkerMsg &msg, Radapter::Reply *reply);
    Radapter::WorkerMsg prepareMsg(const JsonDict &json);
    void handleCommand(Radapter::Command *command, CtxHandle handle);
    void sendMsg(const Radapter::WorkerMsg &msg);
    bool operator==(const CacheContext &other) const {
        return msg().id() == other.msg().id();
    }
private:
    void replyObject(Radapter::Reply &reply);
    void replyPack(Radapter::Reply &reply);

    Connector *m_parent{};
    Radapter::WorkerMsg m_msg{};
};

struct SimpleContext : CacheContext {
    using CacheContext::CacheContext;
    void reply(Radapter::Reply &reply) override;
};

struct PackContext : CacheContext {
    PackContext(const Radapter::WorkerMsg &msgToReply, Connector *parent);
    void reply(Radapter::Reply &reply) override;
protected:
    Radapter::CommandPack *packInMsg();
private:
    Radapter::ReplyPack m_replyPack{};
};

struct ObjectContext : CacheContext {
    ObjectContext(const Radapter::WorkerMsg &msgToReply, Connector *parent, bool doNotReply = false);
    void reply(Radapter::Reply &reply) override;
private:
    bool m_doNotReply;
};

} // namespace Redis

#endif // REDIS_CACHECONTEXT_H
