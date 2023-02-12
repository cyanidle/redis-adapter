#include "rediscacheproducer.h"
#include "formatting/redis/rediscachequeries.h"
#include "radapterlogging.h"
#include "async_context/rediscachecontext.h"
#include "commands/rediscommands.h"
#include "templates/algorithms.hpp"

using namespace Redis;
using namespace Cache;
using namespace Radapter;

CacheProducer::CacheProducer(const Settings::RedisCacheProducer &config, QThread *thread) :
    Connector(config, thread),
    m_objectKey(config.object_hash_key)
{
}

void CacheProducer::onMsg(const Radapter::WorkerMsg &msg)
{
    writeObject(m_objectKey, msg, m_manager.create<ObjectContext>(msg, this, true));
    m_manager.clearDone();
}

void CacheProducer::onCommand(const Radapter::WorkerMsg &msg)
{
    if (msg.command()->is<CommandPack>()) {
        auto packHandle = m_manager.create<PackContext>(msg, this);

    } else {

    }
    m_manager.clearDone();
}

CacheContext &CacheProducer::getCtx(Handle handle)
{
    return m_manager.get(handle);
}


void CacheProducer::requestMultiple(const CommandPack* pack, Handle handle)
{
    if (any_of(pack->commands(), &Command::is<WriteObject>)) {
        auto msgCopy = getCtx(handle).msg();
        auto reply = prepareReply(msgCopy, new ReplyFail("CommandPack Contains WriteObject!"));
        emit sendMsg(reply);
        getCtx(handle).setDone();
    }
    if (!pack->commands().size()) {
        getCtx(handle).fail("Empty command Pack");
        getCtx(handle).setDone();
        return;
    }
    handleCommand(pack->commands().first().data(), handle);
}

void CacheProducer::handleCommand(Radapter::Command *command, Handle handle)
{
    if (command->is<WriteHash>()) {
        writeObject(command->as<WriteHash>()->hash(), command->as<WriteHash>()->flatMap(), handle);
    } else if (command->is<WriteObject>()) {
        writeObject(command->as<WriteObject>()->hashKey(), command->as<WriteObject>()->object(), handle);
    } else if (command->is<WriteKeys>()) {
        writeKeys(command->as<WriteKeys>()->keys(), handle);
    } else if (command->is<WriteSet>()) {
        writeSet(command->as<WriteSet>()->set(), command->as<WriteSet>()->keys(), handle);
    }  else if (command->is<Delete>()) {
        writeSet(command->as<Delete>()->target(), command->as<WriteSet>()->keys(), handle);
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

void CacheProducer::writeObject(const QString &objectKey, const JsonDict &json, Handle handle)
{
    QString keys;
    auto flat = json.flatten();
    for (auto iter{flat.begin()}; iter != flat.end(); ++iter) {
        keys.append(iter.key() + " " + iter.value().toString());
    }
    auto command = QStringLiteral("HMSET %1 %2").arg(objectKey, keys);
    if (runAsyncCommand(&CacheProducer::objectWriteCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("Object write error");
    }
}

void CacheProducer::msetCallback(redisReply *reply, Handle handle)
{

    reDebug() << metaInfo() << "mset status:" << reply->str;
}

void CacheProducer::objectWriteCallback(redisReply *reply, Handle handle)
{

    reDebug() << metaInfo() << "index members updated:" << reply->integer;
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

}
