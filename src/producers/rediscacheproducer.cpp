#include "rediscacheproducer.h"
#include "formatting/redis/rediscachequeries.h"
#include "radapterlogging.h"
#include "async_context/rediscachecontext.h"
#include "commands/rediscommands.h"

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
    writeObject(msg , m_objectKey);
}

void CacheProducer::onCommand(const Radapter::WorkerMsg &msg)
{
    if (msg.command()->is<CommandPack>()) {

    } else {

    }
}

CacheContext &CacheProducer::getCtx(Handle handle)
{
    return m_manager.get(handle);
}

void CacheProducer::handleCommand(Radapter::Command *command, Handle handle)
{
    if (command->is<WriteHash>()) {

    } else if (command->is<WriteObject>()) {

    } else if (command->is<WriteKeys>()) {

    } else if (command->is<WriteSet>()) {

    } else {
        reError() << this << ": Unsupported Command: " << command->metaObject()->className();
    }
}

void CacheProducer::writeKeys(const JsonDict &json, Handle handle)
{
    auto msetCommand = multipleSet(json);
    runAsyncCommand(&CacheProducer::msetCallback, msetCommand, handle);
}

void CacheProducer::writeObject(const JsonDict &json, const QString &objectKey)
{
    //! \todo Finish
    //return runAsyncCommand(&CacheProducer::indexCallback, indexCommand);
}

void CacheProducer::msetCallback(redisReply *reply, Handle handle)
{
    if (!isValidReply(reply))
    {
        reWarn() << metaInfo() << "MSET Error!";
        return;
    }
    reDebug() << metaInfo() << "mset status:" << reply->str;
}

void CacheProducer::indexCallback(redisReply *reply, Handle handle)
{
    if (!isValidReply(reply)) {
        return;
    }
    reDebug() << metaInfo() << "index members updated:" << reply->integer;
}
