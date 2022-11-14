#include "rediscacheproducer.h"
#include "redis-adapter/radapterlogging.h"

using namespace Redis;

CacheProducer::CacheProducer(const QString &host,
                             const quint16 port,
                             const quint16 dbIndex,
                             const QString &indexKey,
                             const Radapter::WorkerSettings &settings) :
    Connector(host, port, dbIndex, settings),
    m_indexKey(indexKey)
{
}

void CacheProducer::onMsg(const Radapter::WorkerMsg &msg)
{
    if (writeIndex(msg.data(), m_indexKey, enqueueMsg(msg)) != REDIS_OK) {
        emit sendMsg(prepareReply(dequeueMsg(msg.id()), "index_write_failed"));
    }
    if (writeKeys(msg.data(), enqueueMsg(msg)) != REDIS_OK) {
        emit sendMsg(prepareReply(dequeueMsg(msg.id()), "keys_write_failed"));
    }
    emit sendMsgWithDirection(prepareMsg(msg), MsgDirection::DirectionToConsumers);
}

int CacheProducer::writeKeys(const Formatters::JsonDict &json, int msgId)
{
    if (json.data().isEmpty()) {
        writeKeysDone(msgId);
        return REDIS_ERR;
    }
    if (!isConnected()) {
        run();
    }

    auto msetCommand = RedisQueryFormatter(json.data()).toMultipleSetCommand();
    return runAsyncCommand(msetCallback, msetCommand, msgId);
}

int CacheProducer::writeIndex(const Formatters::JsonDict &json, const QString &indexKey, int msgId)
{
    if (json.data().isEmpty() || indexKey.isEmpty()) {
        writeIndexDone(msgId);
        return REDIS_ERR;
    }
    if (!isConnected()) {
        run();
    }

    auto indexCommand = RedisQueryFormatter(json.data()).toUpdateIndexCommand(indexKey);
    return runAsyncCommand(indexCallback, indexCommand, msgId);
}

void CacheProducer::msetCallback(redisAsyncContext *context, void *replyPtr, void *args)
{
    auto received = static_cast<CallbackArgs*>(args);
    auto sender = received->sender;
    if (isNullReply(context, replyPtr, sender)
            || isEmptyReply(context, replyPtr))
    {
        return;
    }
    auto reply = static_cast<redisReply *>(replyPtr);
    reDebug() << metaInfo(context).c_str() << "mset status:" << reply->str;
    auto adapter = static_cast<CacheProducer*>(sender);
    adapter->writeKeysDone(received->args.value<quint64>());
    adapter->finishAsyncCommand();
    delete received;
}

void CacheProducer::indexCallback(redisAsyncContext *context, void *replyPtr, void *args)
{
    auto received = static_cast<CallbackArgs*>(args);
    auto sender = received->sender;
    if (isNullReply(context, replyPtr, sender)) {
        return;
    }
    auto reply = static_cast<redisReply *>(replyPtr);
    reDebug() << metaInfo(context).c_str() << "index members updated:" << reply->integer;
    auto adapter = static_cast<CacheProducer*>(sender);
    adapter->writeIndexDone(received->args.value<quint64>());
    adapter->finishAsyncCommand();
    delete received;
}

void CacheProducer::writeKeysDone(int msgId)
{
    emit sendMsg(prepareReply(dequeueMsg(msgId), "keys_write_ok"));
}

void CacheProducer::writeIndexDone(int msgId)
{
    emit sendMsg(prepareReply(dequeueMsg(msgId), "index_write_ok"));
}
