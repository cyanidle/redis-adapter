#include "rediscacheproducer.h"
#include "redis-adapter/radapterlogging.h"

using namespace Redis;

CacheProducer::CacheProducer(const QString &host,
                             const quint16 port,
                             const quint16 dbIndex,
                             const QString &indexKey,
                             const Radapter::WorkerSettings &settings,
                             QThread *thread) :
    Connector(host, port, dbIndex, settings, thread),
    m_indexKey(indexKey)
{
}

void CacheProducer::onMsg(const Radapter::WorkerMsg &msg)
{
    if (writeIndex(msg , m_indexKey) != REDIS_OK) {
        emit sendMsg(prepareReply(msg, new Radapter::ReplyFail));
    }
    if (writeKeys(msg) != REDIS_OK) {
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

int CacheProducer::writeIndex(const JsonDict &json, const QString &indexKey)
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
