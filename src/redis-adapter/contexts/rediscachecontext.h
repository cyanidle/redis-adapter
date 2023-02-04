#ifndef REDIS_CACHECONTEXT_H
#define REDIS_CACHECONTEXT_H

#include "context/contextmanager.h"
#include "radapter-broker/basiccommands.h"
#include "radapter-broker/workermsg.h"

namespace Redis {

class CacheConsumer;
struct CacheContext : Radapter::ContextBase
{
public:
    using CtxHandle = void*;
    CacheContext(const Radapter::WorkerMsg &msgToReply, CacheConsumer *parent);
    Radapter::WorkerMsg &msg();
    const Radapter::WorkerMsg &msg() const;
    void fail(const QString &reason = {});
    virtual void reply(Radapter::Reply &reply) = 0;
    void reply(Radapter::Reply &&reply);
    CacheConsumer *parent();
    bool operator==(const CacheContext &other) const {
        return msg().id() == other.msg().id();
    }
private:
    void replyObject(Radapter::Reply &reply);
    void replyPack(Radapter::Reply &reply);

    CacheConsumer *m_parent{};
    Radapter::WorkerMsg m_msg{};
};

struct SimpleContext : CacheContext {
    SimpleContext(const Radapter::WorkerMsg &msgToReply, CacheConsumer *parent);
    void reply(Radapter::Reply &reply) override;
};

struct PackContext : CacheContext {
    PackContext(const Radapter::WorkerMsg &msgToReply, CacheConsumer *parent);
    void reply(Radapter::Reply &reply) override;
private:
    Radapter::CommandPack *packInMsg();
    Radapter::ReplyPack m_replyPack{};
};

struct ObjectContext : CacheContext {
    ObjectContext(const Radapter::WorkerMsg &msgToReply, CacheConsumer *parent);
    void reply(Radapter::Reply &reply) override;
};

} // namespace Redis

#endif // REDIS_CACHECONTEXT_H
