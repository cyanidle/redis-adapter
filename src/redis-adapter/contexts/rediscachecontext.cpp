#include "rediscachecontext.h"
#include "redis-adapter/commands/rediscommands.h"
#include "redis-adapter/radapterlogging.h"
#include "redis-adapter/consumers/rediscacheconsumer.h"
using namespace Radapter;
using namespace Redis::Cache;
namespace Redis {
CacheContext::CacheContext(const Radapter::WorkerMsg &msgToReply, CacheConsumer *parent) :
    m_parent(parent),
    m_msg(msgToReply)
{
}

WorkerMsg &CacheContext::msg()
{
    return m_msg;
}

const WorkerMsg &CacheContext::msg() const
{
    return m_msg;
}

void CacheContext::fail(const QString &reason)
{
    const auto &checkedReason = reason.isEmpty() ? QStringLiteral("Not Given") : reason;
    reDebug() << parent()->metaInfo() << ": Error! Reason --> "<< checkedReason;
    auto failReply = Radapter::ReplyFail(checkedReason);
    reply(failReply);
}

void CacheContext::reply(Radapter::Reply &&reply)
{
    this->reply(reply);
}

CacheConsumer *CacheContext::parent()
{
    return m_parent;
}

PackContext::PackContext(const Radapter::WorkerMsg &msgToReply, CacheConsumer *parent) :
    CacheContext(msgToReply, parent)
{
    auto pack = msgToReply.command()->as<CommandPack>();
    for (auto &command : pack->commands()) {
        if (command->is<ReadObject>()) {
            throw std::invalid_argument("Cannot ReadIndex inside CommandPack");
        }
    }
}

void PackContext::reply(Radapter::Reply &reply)
{
    m_replyPack.append(reply.newCopy());
    if (m_replyPack.replies().size() >= packInMsg()->commands().size()) {
        auto replyMsg = parent()->prepareReply(msg(), m_replyPack.newCopy());
        emit parent()->sendMsg(replyMsg);
        setDone();
    } else {
        parent()->handleCommand(packInMsg()->commands()[m_replyPack.replies().size()].data(), handle());
    }
}

CommandPack *PackContext::packInMsg()
{
    return msg().command()->as<CommandPack>();
}

SimpleContext::SimpleContext(const Radapter::WorkerMsg &msgToReply, CacheConsumer *parent) :
    CacheContext(msgToReply, parent)
{
}

void SimpleContext::reply(Radapter::Reply &reply)
{
    auto replyMsg = parent()->prepareReply(msg(), reply.newCopy());
    emit parent()->sendMsg(replyMsg);
    setDone();
}


ObjectContext::ObjectContext(const Radapter::WorkerMsg &msgToReply, CacheConsumer *parent) :
    CacheContext(msgToReply, parent)
{
}

void ObjectContext::reply(Radapter::Reply &reply)
{
    if (!reply.ok()) {
        auto replyMsg = parent()->prepareReply(msg(), reply.newCopy());
        emit parent()->sendMsg(replyMsg);
        setDone();
        return;
    }
    auto copy = msg();
    copy.setJson(reply.as<ReplyJson>()->json());
    auto replyMsg = parent()->prepareReply(copy, new ReadObject::WantedReply(true));
    emit parent()->sendMsg(replyMsg);
    setDone();
}











} // namespace Redis
