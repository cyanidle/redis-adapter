#include "rediscacheconsumer.h"
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "redis-adapter/radapterlogging.h"

using namespace Redis;

CacheConsumer::CacheConsumer(const QString &host,
                             const quint16 port,
                             const quint16 dbIndex,
                             const QString &indexKey,
                             const Radapter::WorkerSettings &settings) :
    RedisConnector(host, port, dbIndex, settings),
    m_requestedKeysBuffer{},
    m_indexKey(indexKey),
    m_proto(Radapter::Protocol::instance())
{
}

void CacheConsumer::requestIndex(const QString &indexKey, quint64 msgId)
{
    if (indexKey.isEmpty()) {
        requestIndex(m_indexKey, msgId);
    }
    auto command = RedisQueryFormatter{}.toGetIndexCommand(indexKey);
    runAsyncCommand(readIndexCallback, command, msgId);
}

void CacheConsumer::requestKeys(const Formatters::List &keys, quint64 msgId)
{
    if (keys.isEmpty()) {
        finishKeys(Formatters::Dict{}, msgId);
        return;
    }

    auto command = RedisQueryFormatter{}.toMultipleGetCommand(keys);
    m_requestedKeysBuffer.enqueue(keys);
    runAsyncCommand(readKeysCallback, command, msgId);
}



void CacheConsumer::readIndexCallback(redisAsyncContext *context, void *replyPtr, void *args)
{
    auto cbArgs = static_cast<CallbackArgs*>(args);
    if (isNullReply(context, replyPtr, cbArgs->sender)) {
        return;
    }
    auto reply = static_cast<redisReply *>(replyPtr);
    auto adapter = static_cast<CacheConsumer *>(cbArgs->sender);
    if (reply->elements == 0) {
        reDebug() << metaInfo(context).c_str() << "Empty index.";
        adapter->finishIndex(Formatters::List{}, cbArgs->args.toULongLong());
        return;
    }

    auto indexedKeys = Formatters::List{};
    reDebug() << metaInfo(context).c_str() << "Keys added to index:" << reply->elements;
    for (quint16 i = 0; i < reply->elements; i++) {
        auto keyItem = QString(reply->element[i]->str);
        if (!keyItem.isEmpty()) {
            indexedKeys.append(keyItem);
        }
    }
    adapter->finishIndex(indexedKeys, cbArgs->args.toULongLong());
    delete cbArgs;
}

void CacheConsumer::readKeysCallback(redisAsyncContext *context, void *replyPtr, void *args)
{
    auto cbArgs = static_cast<CallbackArgs*>(args);
    if (isNullReply(context, replyPtr, cbArgs->sender)) {
        return;
    }
    auto reply = static_cast<redisReply *>(replyPtr);
    auto foundEntries = Formatters::List{};
    quint16 keysMatched = 0u;
    for (quint16 i = 0; i < reply->elements; i++) {
        auto entryItem = QString(reply->element[i]->str);
        if (!entryItem.isEmpty()) {
            keysMatched++;
        }
        foundEntries.append(entryItem);
    }
    reDebug() << metaInfo(context).c_str() << "Key entries found:" << keysMatched;
    auto adapter = static_cast<CacheConsumer *>(cbArgs->sender);
    auto resultJson = adapter->mergeWithKeys(foundEntries);
    if (!resultJson.isEmpty()) {
        adapter->finishKeys(resultJson,cbArgs->args.toULongLong());
    }
    adapter->finishAsyncCommand();
    delete cbArgs;
}



void CacheConsumer::finishIndex(const Formatters::List &json, quint64 msgId)
{
    auto reply = prepareReply(dequeueMsg(msgId));
    for (auto &jsonDict : json) {
        reply.setData(jsonDict); // id stays the same, command issuer can check id to receive reply
        emit sendMsg(reply);
    }
    finishAsyncCommand();
}

void CacheConsumer::finishKeys(const Formatters::Dict &json, quint64 msgId)
{
    auto reply = prepareReply(dequeueMsg(msgId));
    reply.setData(json);
    emit sendMsg(reply);
}

Formatters::Dict CacheConsumer::mergeWithKeys(const Formatters::List &entries)
{
    auto requestedKeys = m_requestedKeysBuffer.dequeue();
    if (requestedKeys.isEmpty()) {
        return Formatters::Dict{};
    }
    auto jsonDict = Formatters::Dict{};
    for (quint16 i = 0; i < entries.count(); i++) {
        auto entryValue = entries.at(i).toString();
        if (!entryValue.isEmpty()) {
            auto key = requestedKeys.at(i).toString();
            jsonDict.insert(key, entryValue);
        }
    }
    return jsonDict;
}

void CacheConsumer::onMsg(const Radapter::WorkerMsg &msg)
{
    reDebug() << "CacheConsumer (" << workerName() << "): received generic msg from: " << msg.sender;
}

void CacheConsumer::onCommand(const Radapter::WorkerMsg &msg)
{
    auto requestIndexCommand = m_proto->requestNewJson()->receive(msg).toBool();
    if (requestIndexCommand) {
        requestIndex(m_indexKey, enqueueMsg(msg));
    }
    auto requestKeysCommand = m_proto->requestKeys()->receive(msg).toStringList();
    requestKeys(requestKeysCommand, enqueueMsg(msg));
}
