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

int CacheProducer::writeKeys(const JsonDict &json)
{
    if (json.isEmpty()) {
        return REDIS_ERR;
    }
    auto msetCommand = multipleSet(json);
    return runAsyncCommand(&CacheProducer::msetCallback, msetCommand);
}

int CacheProducer::writeObject(const JsonDict &json, const QString &objectKey)
{
    if (json.isEmpty() || objectKey.isEmpty()) {
        return REDIS_ERR;
    }
    //! \todo Finish
    //return runAsyncCommand(&CacheProducer::indexCallback, indexCommand);
}

void CacheProducer::msetCallback(redisReply *reply)
{
    if (!isValidReply(reply))
    {
        reWarn() << metaInfo() << "MSET Error!";
        return;
    }
    reDebug() << metaInfo() << "mset status:" << reply->str;
}

void CacheProducer::indexCallback(redisReply *reply)
{
    if (!isValidReply(reply)) {
        return;
    }
    reDebug() << metaInfo() << "index members updated:" << reply->integer;
}
