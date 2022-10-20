#include "rediscacheproducer.h"
#include "redis-adapter/radapterlogging.h"

using namespace Redis;

CacheProducer::CacheProducer(const QString &host,
                             const quint16 port,
                             const quint16 dbIndex,
                             const QString &indexKey,
                             const Radapter::WorkerSettings &settings) :
    RedisConnector(host, port, dbIndex, settings),
    m_indexKey(indexKey)
{
}

void CacheProducer::onMsg(const Radapter::WorkerMsg &msg)
{
    writeIndex(msg.data(), m_indexKey, enqueueMsg(msg));
    writeKeys(msg.data(), enqueueMsg(msg));
    auto forwardedToConsumers = prepareMsg(msg.data());
    emit sendMsg(forwardedToConsumers);
}

void CacheProducer::onCommand(const Radapter::WorkerMsg &msg)
{
    if (msg["meta"].toString() == "writeKeys") {
        auto id = enqueueMsg(msg);
        writeKeys(msg["data"], id);
    } else if (msg["meta"].toString() == "writeIndex") {
        auto id = enqueueMsg(msg);
        writeIndex(msg["data"], m_indexKey, id);
    }
}

void CacheProducer::writeKeys(const Formatters::JsonDict &json, int msgId)
{
    if (json.data().isEmpty()) {
        writeKeysDone(msgId);
        return;
    }
    if (!isConnected()) {
        run();
    }

    auto msetCommand = RedisQueryFormatter(json.data()).toMultipleSetCommand();
    runAsyncCommand(msetCallback, msetCommand, msgId);
}

void CacheProducer::writeIndex(const Formatters::JsonDict &json, const QString &indexKey, int msgId)
{
    if (json.data().isEmpty() || indexKey.isEmpty()) {
        writeIndexDone(msgId);
        return;
    }
    if (!isConnected()) {
        run();
    }

    auto indexCommand = RedisQueryFormatter(json.data()).toUpdateIndexCommand(indexKey);
    runAsyncCommand(indexCallback, indexCommand, msgId);
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
    emit sendMsg(prepareReply(dequeueMsg(msgId)));
}

void CacheProducer::writeIndexDone(int msgId)
{
    emit sendMsg(prepareReply(dequeueMsg(msgId)));
}
