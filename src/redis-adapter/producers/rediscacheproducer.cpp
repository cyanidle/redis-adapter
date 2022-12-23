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
        emit sendMsg(prepareReply(msg, "index_write_failed"));
    }
    if (writeKeys(msg.data(), msg) != REDIS_OK) {
        emit sendMsg(prepareReply(msg, "keys_write_failed"));
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
    return runAsyncCommandWithMsg(msetCallback, msetCommand, msg);
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
    return runAsyncCommandWithMsg(indexCallback, indexCommand, msg);
}

void CacheProducer::msetCallback(redisAsyncContext *context, void *replyPtr, void *sender, const Radapter::WorkerMsg &msg)
{
    auto adapter = static_cast<CacheProducer*>(sender);
    if (isNullReply(context, replyPtr, sender)
            || isEmptyReply(context, replyPtr))
    {
        return;
    }
    auto reply = static_cast<redisReply *>(replyPtr);
    reDebug() << metaInfo(context).c_str() << "mset status:" << reply->str;
    adapter->writeKeysDone(msg);
}

void CacheProducer::indexCallback(redisAsyncContext *context, void *replyPtr, void *sender, const Radapter::WorkerMsg &msg)
{
    auto adapter = static_cast<CacheProducer*>(sender);
    if (isNullReply(context, replyPtr, sender)) {
        return;
    }
    auto reply = static_cast<redisReply *>(replyPtr);
    reDebug() << metaInfo(context).c_str() << "index members updated:" << reply->integer;
    adapter->writeIndexDone(msg);
}

void CacheProducer::writeKeysDone(const Radapter::WorkerMsg &msg)
{
    emit sendMsg(prepareReply(msg, "keys_write_ok"));
}

void CacheProducer::writeIndexDone(const Radapter::WorkerMsg &msg)
{
    emit sendMsg(prepareReply(msg, "index_write_ok"));
}
