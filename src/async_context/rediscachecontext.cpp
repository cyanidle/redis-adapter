#include "rediscachecontext.h"
#include "commands/rediscommands.h"
#include "producers/rediscacheproducer.h"
#include "radapterlogging.h"
#include "consumers/rediscacheconsumer.h"
using namespace Radapter;
using namespace Redis::Cache;
namespace Redis {
CacheContext::CacheContext(const Radapter::WorkerMsg &msgToReply, Connector *parent) :
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
    reDebug() << m_parent->metaInfo() << ": Error! Reason --> "<< checkedReason;
    auto failReply = Radapter::ReplyFail(checkedReason);
    reply(failReply);
}

void CacheContext::reply(Radapter::Reply &&reply)
{
    this->reply(reply);
}

WorkerMsg CacheContext::prepareReply(const Radapter::WorkerMsg &msg, Radapter::Reply *reply)
{
    auto prod = m_parent->as<CacheProducer>();
    auto cons = m_parent->as<CacheConsumer>();
    if (prod) {
        return prod->prepareReply(msg, reply);
    } else if (cons) {
        return cons->prepareReply(msg, reply);
    } else {
        throw std::runtime_error("Context Critical Error!");
    }
}

WorkerMsg CacheContext::prepareMsg(const JsonDict &json)
{
    auto prod = m_parent->as<CacheProducer>();
    auto cons = m_parent->as<CacheConsumer>();
    if (prod) {
        return prod->prepareMsg(json);
    } else if (cons) {
        return cons->prepareMsg(json);
    } else {
        throw std::runtime_error("Context Critical Error!");
    }
}

void CacheContext::handleCommand(Radapter::Command *command, CtxHandle handle)
{
    auto prod = m_parent->as<CacheProducer>();
    auto cons = m_parent->as<CacheConsumer>();
    if (prod) {
        return prod->handleCommand(command, handle);
    } else if (cons) {
        return cons->handleCommand(command, handle);
    } else {
        throw std::runtime_error("Context Critical Error!");
    }
}

void CacheContext::sendMsg(const Radapter::WorkerMsg &msg)
{
    auto prod = m_parent->as<CacheProducer>();
    auto cons = m_parent->as<CacheConsumer>();
    if (prod) {
        return prod->sendMsg(msg);
    } else if (cons) {
        return cons->sendMsg(msg);
    } else {
        throw std::runtime_error("Context Critical Error!");
    }
}

PackContext::PackContext(const Radapter::WorkerMsg &msgToReply, Connector *parent) :
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
        auto replyMsg = prepareReply(msg(), m_replyPack.newCopy());
        sendMsg(replyMsg);
        setDone();
    } else {
        handleCommand(packInMsg()->commands()[m_replyPack.replies().size()].data(), handle());
    }
}

CommandPack *PackContext::packInMsg()
{
    return msg().command()->as<CommandPack>();
}

void SimpleContext::reply(Radapter::Reply &reply)
{
    auto replyMsg = prepareReply(msg(), reply.newCopy());
    sendMsg(replyMsg);
    setDone();
}


ObjectContext::ObjectContext(const Radapter::WorkerMsg &msgToReply, Connector *parent, bool doNotReply) :
    CacheContext(msgToReply, parent),
    m_doNotReply(doNotReply)
{
}

void ObjectContext::reply(Radapter::Reply &reply)
{
    if (!reply.ok()) {
        if (m_doNotReply) {
            reDebug() << "Object read error";
            return;
        }
        auto replyMsg = prepareReply(msg(), reply.newCopy());
        sendMsg(replyMsg);
        setDone();
        return;
    }
    WorkerMsg toSend;
    if (m_doNotReply) {
        toSend = prepareMsg(reply.as<ReplyJson>()->json());
    } else {
        auto copy = msg();
        copy.setJson(reply.as<ReplyJson>()->json());
        toSend = prepareReply(copy, new ReadObject::WantedReply(true));
    }
    sendMsg(toSend);
    setDone();
}

} // namespace Redis
