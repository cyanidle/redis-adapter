#include "rediscacheproducer.h"
#include "redis-adapter/radapterlogging.h"
#include "redis-adapter/formatters/redisqueryformatter.h"

using namespace Redis;

CacheProducer::CacheProducer(const Settings::RedisCacheProducer &config, QThread *thread) :
    Connector(config, thread),
    m_indexKey(config.object_hash_key)
{
}

void CacheProducer::onMsg(const Radapter::WorkerMsg &msg)
{
    if (writeObject(msg , m_indexKey) != REDIS_OK) {
        emit sendMsg(prepareReply(msg, new Radapter::ReplyFail));
    }
}

int CacheProducer::writeKeys(const JsonDict &json)
{
    if (json.isEmpty()) {
        return REDIS_ERR;
    }
    auto msetCommand = RedisQueryFormatter(json).toMultipleSetCommand();
    return runAsyncCommand(&CacheProducer::msetCallback, msetCommand);
}

int CacheProducer::writeObject(const JsonDict &json, const QString &indexKey)
{
    if (json.isEmpty() || indexKey.isEmpty()) {
        return REDIS_ERR;
    }
    auto indexCommand = RedisQueryFormatter(json).toUpdateIndexCommand(indexKey);
    return runAsyncCommand(&CacheProducer::indexCallback, indexCommand);
}

void CacheProducer::msetCallback(redisReply *reply)
{
    if (!isValidReply(reply))
    {
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
