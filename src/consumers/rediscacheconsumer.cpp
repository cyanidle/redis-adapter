#include "rediscacheconsumer.h"
#include "radapterlogging.h"
#include "commands/rediscommands.h"
#include "include/redisconstants.h"
#include "templates/algorithms.hpp"

using namespace Redis;
using namespace Cache;
using namespace Radapter;

CacheConsumer::CacheConsumer(const Settings::RedisCacheConsumer &config, QThread *thread) :
    Connector(config, thread),
    m_objectKey(config.object_hash_key)
{
    if (!config.object_hash_key->isEmpty()) {
        m_objectRead = new QTimer(this);
        m_objectRead->callOnTimeout(this, &CacheConsumer::requestObjectSimple);
        m_objectRead->setInterval(config.update_rate);
    }
    connect(this, &Connector::disconnected, this, &CacheConsumer::onDisconnect);
}

void CacheConsumer::onDisconnect()
{
    m_manager.forEach(&CacheContext::fail, "Disconnected");
    m_manager.clearAll();
}

void CacheConsumer::requestObject(const QString &objectKey, CtxHandle handle)
{
    auto command = QStringLiteral("HGETALL ") + objectKey;
    if (runAsyncCommand(&CacheConsumer::readObjectCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("Object request fail");
    }
}

void CacheConsumer::readObjectCallback(redisReply *reply, CtxHandle handle)
{
    auto result = parseHashReply(reply);
    workerInfo(this) << ": Found object fields: " << result.count();
    if (result.isEmpty()) {
        getCtx(handle).fail("Empty object hash!");
    }
    getCtx(handle).reply(ReplyJson(JsonDict{result}));
}

void CacheConsumer::requestKeys(const QStringList &keys, CtxHandle handle)
{
    auto command = QStringLiteral("MGET ") + keys.join(" ");
    if (runAsyncCommand(&CacheConsumer::readKeysCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("MGET Error");
    }
}

void CacheConsumer::readKeysCallback(redisReply *reply, CtxHandle handle)
{
    auto foundEntries = parseReply(reply).toStringList();
    workerInfo(this) << "Key entries found:" << foundEntries.size();
    getCtx(handle).reply(ReadKeys::WantedReply(foundEntries));
}

void CacheConsumer::requestKey(const QString &key, CtxHandle handle)
{
    auto command = QStringLiteral("GET ") + key;
    if (runAsyncCommand(&CacheConsumer::readKeyCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("GET Error");
    }
}

void CacheConsumer::readKeyCallback(redisReply *replyPtr, CtxHandle handle)
{
    auto foundKey = parseReply(replyPtr).toString();
    workerInfo(this) << "Key found:" << foundKey;
    getCtx(handle).reply(ReadKey::WantedReply(foundKey));
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
    workerInfo(this) << "Hash entries found:" << result.size();
    getCtx(handle).reply(ReplyHash(result));
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
    workerInfo(this) << "Set entries found:" << parsed.size();
    getCtx(handle).reply(ReadSet::WantedReply(parsed));
}

void CacheConsumer::requestObjectSimple()
{
    auto command = QStringLiteral("HGETALL ") + m_objectKey;
    auto handle = m_manager.create<SimpleMsgContext>(this);
    if (runAsyncCommand(&CacheConsumer::readObjectCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("Object request fail");
    }
}

void CacheConsumer::handleCommand(const Radapter::Command *command, CtxHandle handle)
{
    if (command->is<ReadKeys>()) {
        requestKeys(command->as<ReadKeys>()->keys(), handle);
    } else if (command->is<ReadKey>()) {
        requestKey(command->as<ReadKey>()->key(), handle);
    } else if (command->is<ReadSet>()) {
        requestSet(command->as<ReadSet>()->set(), handle);
    } else if (command->is<ReadHash>()) {
        requestHash(command->as<ReadHash>()->hash(), handle);
    } else if (command->is<ReadObject>()) {
        requestObject(command->as<ReadObject>()->key(), handle);
    } else {
        getCtx(handle).fail(QStringLiteral("Command type unsupported: ") + command->metaObject()->className());
    }
}

CacheContext &CacheConsumer::getCtx(CtxHandle handle)
{
    return m_manager.get(handle);
}

void CacheConsumer::onCommand(const WorkerMsg &msg)
{
    if (msg.command()->is<CommandPack>()) {
        handleCommand(msg.command()->as<CommandPack>()->first(), m_manager.create<PackContext>(msg, this));
    } else if (msg.command()->is<CommandTriggerRead>()) {
        requestObject(m_objectKey, m_manager.create<SimpleMsgContext>(this));
    } else {
        handleCommand(msg.command(), m_manager.create<SimpleContext>(msg, this));
    }
    m_manager.clearDone();
}

void CacheConsumer::onRun()
{
    if (m_objectRead) {
        m_objectRead->start();
    }
    Connector::onRun();
}

