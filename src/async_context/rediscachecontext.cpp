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
    m_prod(qobject_cast<CacheProducer*>(parent)),
    m_cons(qobject_cast<CacheConsumer*>(parent))
{
    if (!m_prod && !m_cons) {
        throw std::invalid_argument("Context must be used in cache consumer or producer only!");
    }
}

void CacheContext::fail(const QString &reason)
{
    const auto &checkedReason = reason.isEmpty() ? QStringLiteral("Not Given") : reason;
    Worker *worker;
    if (m_cons){
        worker = m_cons;
    } else {
        worker = m_prod;
    }
    workerError(worker) << "Operation Error! Reason --> "<< checkedReason;
    reply(Radapter::ReplyFail(checkedReason));
}

void CacheContext::reply(Radapter::Reply &&reply)
{
    this->reply(reply);
}

WorkerMsg CacheContext::prepareReply(const Radapter::WorkerMsg &msg, Radapter::Reply *reply)
{
    if (m_prod) {
        return m_prod->prepareReply(msg, reply);
    } else {
        return m_cons->prepareReply(msg, reply);
    }
}

WorkerMsg CacheContext::prepareMsg(const JsonDict &json)
{
    if (m_prod) {
        return m_prod->prepareMsg(json);
    } else {
        return m_cons->prepareMsg(json);
    }
}

void CacheContext::handleCommand(Radapter::Command *command, Handle handle)
{
    if (m_prod) {
        return m_prod->handleCommand(command, handle);
    } else {
        return m_cons->handleCommand(command, handle);
    }
}

void CacheContext::sendMsg(const Radapter::WorkerMsg &msg)
{
    if (m_prod) {
        emit m_prod->sendMsg(msg);
    } else {
        emit m_cons->sendMsg(msg);
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
    if (m_replyPack.size() >= packInMsg()->size() || !reply.ok()) {
        while (m_replyPack.size() < packInMsg()->size()) {
            m_replyPack.append(new ReplyFail);
        }
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

void SimpleMsgContext::reply(Radapter::Reply &reply)
{
    auto asJson = reply.as<ReplyJson>();
    if (!asJson || !asJson->ok()) return;
    sendMsg(prepareMsg(asJson->json()));
    setDone();
}

void NoReplyContext::reply(Radapter::Reply &reply)
{
    Q_UNUSED(reply);
    setDone();
}

} // namespace Redis
