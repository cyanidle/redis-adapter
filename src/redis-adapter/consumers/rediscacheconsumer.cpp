#include "rediscacheconsumer.h"
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "redis-adapter/radapterlogging.h"

using namespace Redis;

CacheConsumer::CacheConsumer(const QString &host,
                             const quint16 port,
                             const quint16 dbIndex,
                             const QString &indexKey,
                             const Radapter::WorkerSettings &settings,
                             QThread *thread) :
    Connector(host, port, dbIndex, settings, thread),
    m_requestedKeysBuffer{},
    m_indexKey(indexKey)
{
}

void CacheConsumer::requestIndex(const QString &indexKey, const Radapter::WorkerMsg &msg)
{
    if (indexKey.isEmpty()) {
        requestIndex(m_indexKey, msg);
    }
    auto command = RedisQueryFormatter{}.toGetIndexCommand(indexKey);
    runAsyncCommandWithMsg(readIndexCallback, command, msg);
}

void CacheConsumer::requestKeys(const Formatters::List &keys, const Radapter::WorkerMsg &msg)
{
    if (keys.isEmpty()) {
        finishKeys(Formatters::Dict{}, msg);
        return;
    }

    auto command = RedisQueryFormatter{}.toMultipleGetCommand(keys);
    m_requestedKeysBuffer.enqueue(keys);
    runAsyncCommandWithMsg(readKeysCallback, command, msg);
}

void CacheConsumer::readIndexCallback(redisAsyncContext *context, void *replyPtr, void *sender, const Radapter::WorkerMsg &msg)
{
    if (isNullReply(context, replyPtr, sender)) {
        return;
    }
    auto reply = static_cast<redisReply *>(replyPtr);
    auto adapter = static_cast<CacheConsumer *>(sender);
    if (reply->elements == 0) {
        reDebug() << metaInfo(context).c_str() << "Empty index.";
        adapter->finishIndex(Formatters::List{}, msg);
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
    adapter->finishIndex(indexedKeys, msg);
}

void CacheConsumer::readKeysCallback(redisAsyncContext *context, void *replyPtr, void *sender, const Radapter::WorkerMsg &msg)
{
    if (isNullReply(context, replyPtr, sender)) {
        return;
    }
    auto adapter = static_cast<CacheConsumer*>(sender);
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
    auto resultJson = adapter->mergeWithKeys(foundEntries);
    adapter->finishKeys(resultJson, msg);
}

void CacheConsumer::finishIndex(const Formatters::List &json, const Radapter::WorkerMsg &msg)
{
    auto reply = prepareReply(msg);
    for (auto &jsonDict : json) {
        reply.setJson(jsonDict.toMap()); // id stays the same, command issuer can check id to receive reply
        emit sendMsg(reply);
    }
}

void CacheConsumer::finishKeys(const Formatters::Dict &json, const Radapter::WorkerMsg &msg)
{
    auto reply = prepareReply(msg);
    reply.setJson(json);
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

void CacheConsumer::onCommand(const Radapter::WorkerMsg &msg)
{
    auto requestedJson = msg.serviceData(Radapter::WorkerMsg::ServiceRequestJson);
    auto requestedKeys = msg.serviceData(Radapter::WorkerMsg::ServiceRequestKeys);
    if (requestedJson.isValid()) {
        requestIndex(m_indexKey, msg);
    } else if (requestedKeys.isValid()) {
        requestKeys(requestedKeys.toStringList(), msg);
    }
}
