#include "rediscacheconsumer.h"
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "redis-adapter/radapterlogging.h"
#include "radapter-broker/reply.h"
#include "redis-adapter/commands/rediscommands.h"
#include "redis-adapter/include/redismessagekeys.h"

using namespace Redis;
using namespace Cache;
using namespace Radapter;

CacheConsumer::CacheConsumer(const Settings::RedisCacheConsumer &config, QThread *thread) :
    Connector(config, thread),
    m_indexKey(config.index_key)
{
}

void CacheConsumer::requestObject(const QString &objectKey, CtxHandle handle)
{
    auto command = QStringLiteral("HGETALL ") + objectKey;
    if (runAsyncCommand(&CacheConsumer::readObjectCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail(metaInfo());
    }
}

void CacheConsumer::readObjectCallback(redisReply *reply, CtxHandle handle)
{
    auto result = parseHashReply(reply);
    if (result.isEmpty()) {
        getCtx(handle).fail("Empty index");
    }
    auto keysToRead = QStringList();
    //! \todo finish
    for (auto iter = result.begin(); iter != result.end(); ++iter) {
        auto &key = iter.key();
        auto stringValue = iter.value().toString();

    }
    getCtx(handle).executeNextForIndex();
}

void CacheConsumer::requestKeys(const QStringList &keys, CtxHandle handle)
{
    auto command = QStringLiteral("MGET ") + keys.join(" ");
    if (runAsyncCommand(&CacheConsumer::readKeysCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("MGET Error");
    }
}

void CacheConsumer::readKeysCallback(redisReply *replyPtr, CtxHandle handle)
{
    auto foundEntries = parseReply(replyPtr).toStringList();
    reDebug() << metaInfo() << "Key entries found:" << foundEntries.size();
    if (foundEntries.isEmpty()) {
        getCtx(handle).fail("Empty Keys Read");
    } else {
        auto toReply = ReadKeys::WantedReply(foundEntries);
        getCtx(handle).reply(toReply);
    }
}

void CacheConsumer::requestHash(const QString &hash, CtxHandle handle)
{
    auto command = QStringLiteral("HGETALL ") + hash;
    if (runAsyncCommand(&CacheConsumer::readHashCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("HGETALL Error");
    }
}

void CacheConsumer::readHashCallback(redisReply *replyPtr, CtxHandle handle)
{
    auto result = parseHashReply(replyPtr);
    if (result.isEmpty()) {
        getCtx(handle).fail("Empty Hash");
    } else {
        getCtx(handle).reply(ReplyHash(result));
    }
}

void CacheConsumer::requestSet(const QString &setKey, CtxHandle handle)
{
    auto command = QStringLiteral("SMEMBERS ") + setKey;
    if (runAsyncCommand(&CacheConsumer::readSetCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("SMEMBERS Error");
    }
}

void CacheConsumer::readSetCallback(redisReply *replyPtr, CtxHandle handle)
{
    auto parsed = parseReply(replyPtr).toStringList();
    if (parsed.isEmpty()) {
        getCtx(handle).fail("Empty set");
    } else {
        auto toReply = ReadSet::WantedReply(parsed);
        getCtx(handle).reply(toReply);
    }
}

void CacheConsumer::handleCommand(const Radapter::Command *command, CtxHandle handle)
{
    if (command->is<ReadObject>()) {
        requestObject(command->as<ReadObject>()->key(), handle);
    } else if (command->is<ReadKeys>()) {
        requestKeys(command->as<ReadKeys>()->keys(), handle);
    } else if (command->is<ReadSet>()) {
        requestSet(command->as<ReadSet>()->set(), handle);
    } else if (command->is<CommandPack>()) {
        requestMultiple(command->as<CommandPack>(), handle);
    } else if (command->is<ReadHash>()) {
        requestHash(command->as<ReadHash>()->hash(), handle);
    } else {
        getCtx(handle).fail(QStringLiteral("Command type unsupported: ") + command->metaObject()->className());
    }
}

void CacheConsumer::requestMultiple(const CommandPack* pack, CtxHandle handle)
{
    for (auto &command : pack->commands()) {
        handleCommand(command.data(), handle);
    }
}

CacheContext::CacheContext(const Radapter::WorkerMsg &msgToReply, CacheConsumer *parent) :
    m_parent(parent),
    m_msg(msgToReply)
{
    if (msgToReply.command()->is<ReadObject>()) {
        m_type = IndexRead;
    }
    if (msgToReply.command()->is<CommandPack>()) {
        auto pack = msgToReply.command()->as<CommandPack>();
        for (auto &command : pack->commands()) {
            if (command->is<ReadObject>()) {
                throw std::invalid_argument("Cannot ReadIndex inside CommandPack");
            }
        }
        m_type = PackRead;
    } else {
        m_type = SimpleRead;
    }
}

void CacheContext::addNestedObjectResult(Command &nextStep, const QString &key, const QString &value)
{
    if (m_type != IndexRead) throw std::runtime_error("Non index read attemt to add commands");
}


void CacheContext::fail(const QString &reason)
{
    reDebug() << m_parent->metaInfo() << ": Error! Reason --> "<< (reason.isEmpty() ? QStringLiteral("Not Given") : reason);
    auto failReply = Radapter::ReplyFail(reason);
    reply(failReply);
}

void CacheContext::reply(Radapter::Reply &reply)
{
    if (m_type == SimpleRead) {
        replySimple(reply);
    } else if (m_type == IndexRead) {
        replyIndex(reply);
    } else if (m_type == PackRead) {
        replyPack(reply);
    } else {
        throw std::runtime_error("Cache context type uninitialized!");
    }
}

CommandPack *CacheContext::packFromMsg()
{
    return m_msg.command()->as<CommandPack>();
}

void CacheContext::executeNextForIndex()
{
    //! \todo exec all, but wait to end async
    auto commandsLeft = m_pack.commands().count();
    if (m_executedCount + m_inReply >= commandsLeft) {
        return;
    }
    if (++m_executedCount >= commandsLeft) {
        auto replyMsg = m_parent->prepareReply(m_msg, new ReplyWithJson(m_replyPack.ok()));
        emit m_parent->sendMsg(replyMsg);
    } else {
        m_parent->handleCommand(m_pack.commands()[m_executedCount].data(), m_parent->getHandle(*this));
    }
}

void CacheContext::replyIndex(Radapter::Reply &reply)
{
    --m_inReply;
    if (m_pack.commands()[m_executedCount]->replyOk(&reply)) {
        executeNextForIndex();
    } else {
        auto replyMsg = m_parent->prepareReply(m_msg, reply.newCopy());
        emit m_parent->sendMsg(replyMsg);
        m_isDone = true;
    }
}

void CacheContext::replySimple(Radapter::Reply &reply)
{
    auto replyMsg = m_parent->prepareReply(m_msg, reply.newCopy());
    emit m_parent->sendMsg(replyMsg);
    m_isDone = true;
}

void CacheContext::replyPack(Radapter::Reply &reply)
{
    m_replyPack.append(reply.newCopy());
    if (++m_executedCount >= packFromMsg()->commands().count()) {
        auto replyMsg = m_parent->prepareReply(m_msg, m_replyPack.newCopy());
        emit m_parent->sendMsg(replyMsg);
        m_isDone = true;
    } else {
        m_parent->handleCommand(packFromMsg()->commands()[m_executedCount].data(), m_parent->getHandle(*this));
    }
}

CacheContext &CacheConsumer::getCtx(CtxHandle handle)
{
    return m_manager.get(handle);
}

void CacheConsumer::onCommand(const WorkerMsg &msg)
{
    handleCommand(msg.command(), m_manager.manage(CacheContext(msg, this)));
    m_manager.clearBasedOn(&CacheContext::isDone);
}

CacheConsumer::CtxHandle CacheConsumer::getHandle(const CacheContext &ctx)
{
    return m_manager.getHandle(ctx);
}
