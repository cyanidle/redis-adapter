#include "rediscacheproducer.h"
#include "formatting/redis/rediscachequeries.h"
#include "async_context/rediscachecontext.h"
#include "commands/rediscommands.h"
#include "settings/redissettings.h"

using namespace Redis;
using namespace Cache;
using namespace Radapter;

CacheProducer::CacheProducer(const Settings::RedisCacheProducer &config, QThread *thread) :
    Connector(config, thread),
    m_objectKey(config.object_hash_key)
{
    connect(this, &CacheProducer::disconnected, this, &CacheProducer::onDisconnect);
}

void CacheProducer::onMsg(const Radapter::WorkerMsg &msg)
{
    m_manager.clearDone();
    if (m_objectKey.isEmpty()) {
        workerError(this) << "To write plain Jsons, specify 'object_hash_key' in config!";
        return;
    }
    writeObject(m_objectKey, msg, m_manager.create<NoReplyContext>(this));
}

void CacheProducer::onCommand(const Radapter::WorkerMsg &msg)
{
    if (msg.command()->is<CommandPack>()) {
        handleCommand(msg.command()->as<CommandPack>()->first(), m_manager.create<PackContext>(msg, this));
    } else {
        handleCommand(msg.command(), m_manager.create<SimpleContext>(msg, this));
    }
    m_manager.clearDone();
}

void CacheProducer::onDisconnect()
{
    m_manager.forEach(&CacheContext::fail, "Disconnected");
    m_manager.clearAll();
}

CacheContext &CacheProducer::getCtx(Handle handle)
{
    return m_manager.get(handle);
}

void CacheProducer::handleCommand(const Radapter::Command *command, Handle handle)
{
    auto hashCmd = command->as<WriteHash>();
    auto objectCmd = command->as<WriteObject>();
    auto keysCmd = command->as<WriteKeys>();
    auto setCmd = command->as<WriteSet>();
    auto delCmd = command->as<Delete>();
    if (hashCmd) {
        writeObject(hashCmd->hash(), hashCmd->flatMap(), handle);
    } else if (objectCmd) {
        writeObject(objectCmd->hashKey(), objectCmd->object(), handle);
    } else if (keysCmd) {
        writeKeys(keysCmd->keys(), handle);
    } else if (setCmd) {
        writeSet(setCmd->set(), setCmd->keys(), handle);
    }  else if (delCmd) {
        del(delCmd->target(), handle);
    } else {
        getCtx(handle).fail(QStringLiteral("Unsupported Command: ") + command->metaObject()->className());
    }
}

void CacheProducer::writeKeys(const QVariantMap &keys, Handle handle)
{
    auto msetCommand = toMultipleSet(keys);
    if (runAsyncCommand(&CacheProducer::msetCallback, msetCommand, handle) != REDIS_OK) {
        getCtx(handle).fail("Write Keys error");
    }
}

void CacheProducer::msetCallback(redisReply *reply, Handle handle)
{
    auto status = parseReply(reply);
    if (status == "OK") {
        getCtx(handle).reply(ReplyOk());
    } else {
        getCtx(handle).fail("Write set fail; Status: " + status.toString());
    }
}

void CacheProducer::writeObject(const QString &objectKey, const JsonDict &json, Handle handle)
{
    QString keys;
    auto flat = json.flatten();
    for (auto iter{flat.begin()}; iter != flat.end(); ++iter) {
        keys.append(QStringLiteral(" %1 %2").arg(iter.key(), iter.value().toString()));
    }
    auto command = QStringLiteral("HMSET %1 %2").arg(objectKey, keys);
    if (runAsyncCommand(&CacheProducer::objectWriteCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("Object write error");
    }
}

void CacheProducer::objectWriteCallback(redisReply *reply, Handle handle)
{
    auto status = parseReply(reply);
    if (status == "OK") {
        getCtx(handle).reply(ReplyOk());
    } else {
        getCtx(handle).fail("Write set fail; Status: " + status.toString());
    }
}


void CacheProducer::writeSet(const QString &set, const QStringList &keys, Handle handle)
{
    auto sadd = QStringLiteral("SADD %1 %2").arg(set, keys.join(" "));
    if (runAsyncCommand(&CacheProducer::writeSetCallback, sadd, handle) != REDIS_OK) {
        getCtx(handle).fail("Set update error");
    }
}

void CacheProducer::writeSetCallback(redisReply *reply, Handle handle)
{
    auto status = parseReply(reply);
    if (status == "OK") {
        getCtx(handle).reply(ReplyOk());
    } else {
        getCtx(handle).fail("Write set fail; Status: " + status.toString());
    }
}

void CacheProducer::del(const QString &target, Handle handle)
{
    auto delCmd = QStringLiteral("DEL %1").arg(target);
    if (runAsyncCommand(&CacheProducer::delCallback, delCmd, handle) != REDIS_OK) {
        getCtx(handle).fail("Delete error");
    }
}

void CacheProducer::delCallback(redisReply *reply, Handle handle)
{
    auto status = parseReply(reply);
    if (status == "OK") {
        getCtx(handle).reply(ReplyOk());
    } else {
        getCtx(handle).fail("Write set fail; Status: " + status.toString());
    }
}
