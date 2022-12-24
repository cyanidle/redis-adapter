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
    if (writeIndex(msg.data(), m_indexKey, msg) != REDIS_OK) {
        emit sendMsg(prepareReply(msg, Radapter::WorkerMsg::ReplyFail));
    }
    if (writeKeys(msg.data(), msg) != REDIS_OK) {
        emit sendMsg(prepareReply(msg, Radapter::WorkerMsg::ReplyFail));
    }
    emit sendMsg(prepareMsg(msg));
}

int CacheProducer::writeKeys(const Formatters::JsonDict &json, const Radapter::WorkerMsg &msg)
{
    if (json.data().isEmpty()) {
        writeKeysDone(msg);
        return REDIS_ERR;
    }
    if (!isConnected()) {
        run();
    }
    auto msetCommand = RedisQueryFormatter(json.data()).toMultipleSetCommand();
    auto id = enqueueMsg(msg);
    auto status = runAsyncCommand(&CacheProducer::msetCallback, msetCommand, id);
    if (status != REDIS_OK) {
        disposeId(id);
    }
    return status;
}

int CacheProducer::writeIndex(const Formatters::JsonDict &json, const QString &indexKey, const Radapter::WorkerMsg &msg)
{
    if (json.data().isEmpty() || indexKey.isEmpty()) {
        writeIndexDone(msg);
        return REDIS_ERR;
    }
    if (!isConnected()) {
        run();
    }
    auto indexCommand = RedisQueryFormatter(json.data()).toUpdateIndexCommand(indexKey);
    auto id = enqueueMsg(msg);
    auto status = runAsyncCommand(&CacheProducer::indexCallback, indexCommand, id);
    if (status != REDIS_OK) {
        disposeId(id);
    }
    return status;
}

void CacheProducer::msetCallback(redisAsyncContext *context, redisReply *reply, void *msgId)
{
    auto msg = dequeueMsg(msgId);
    if (isNullReply(context, reply)
            || isEmptyReply(context, reply))
    {
        return;
    }
    reDebug() << metaInfo(context).c_str() << "mset status:" << reply->str;
    writeKeysDone(msg);
}

void CacheProducer::indexCallback(redisAsyncContext *context, redisReply *reply, void *msgId)
{
    auto msg = dequeueMsg(msgId);
    if (isNullReply(context, reply)) {
        return;
    }
    reDebug() << metaInfo(context).c_str() << "index members updated:" << reply->integer;
    writeIndexDone(msg);
}

void CacheProducer::writeKeysDone(const Radapter::WorkerMsg &msg)
{
    emit sendMsg(prepareReply(msg, Radapter::WorkerMsg::ReplyOk));
}

void CacheProducer::writeIndexDone(const Radapter::WorkerMsg &msg)
{
    emit sendMsg(prepareReply(msg, Radapter::WorkerMsg::ReplyOk));
}
