#include "rediscachecontext.h"
#include "commands/rediscommands.h"
#include "producers/rediscacheproducer.h"
#include "radapterlogging.h"
#include "consumers/rediscacheconsumer.h"
#include <QTimer>
using namespace Radapter;
using namespace Redis::Cache;
namespace Redis {
CacheContextWithReply::CacheContextWithReply(const Radapter::WorkerMsg &msgToReply, Connector *parent) :
    CacheContext(parent),
    m_msg(msgToReply)
{
}

WorkerMsg &CacheContextWithReply::msg()
{
    return m_msg;
}

const WorkerMsg &CacheContextWithReply::msg() const
{
    return m_msg;
}

CacheContext::CacheContext(Connector *parent) :
    m_parent(parent)
{
}

void CacheContext::fail(const QString &reason)
{
    const auto &checkedReason = reason.isEmpty() ? QStringLiteral("Not Given") : reason;
    workerError(m_parent) << ": Error! Reason --> "<< checkedReason;
    auto failReply = Radapter::ReplyFail(checkedReason);
    reply(failReply);
    setDone();
}

void CacheContext::reply(Radapter::Reply &&reply)
{
    this->reply(reply);
}

WorkerMsg CacheContextWithReply::prepareReply(const Radapter::WorkerMsg &msg, Radapter::Reply *reply)
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

WorkerMsg CacheContextWithReply::prepareMsg(const JsonDict &json)
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

void CacheContextWithReply::handleCommand(Radapter::Command *command, CtxHandle handle)
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

void CacheContextWithReply::sendMsg(const Radapter::WorkerMsg &msg)
{
    auto prod = m_parent->as<CacheProducer>();
    auto cons = m_parent->as<CacheConsumer>();
    if (prod) {
        emit prod->sendMsg(msg);
    } else if (cons) {
        emit cons->sendMsg(msg);
    } else {
        throw std::runtime_error("Context Critical Error!");
    }
}

PackContext::PackContext(const Radapter::WorkerMsg &msgToReply, Connector *parent) :
    CacheContextWithReply(msgToReply, parent)
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

void NoReplyContext::reply(Radapter::Reply &reply)
{
    Q_UNUSED(reply);
    setDone();
}

} // namespace Redis
