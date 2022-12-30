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
    auto command = RedisQueryFormatter::toGetIndexCommand(indexKey);
    auto id = enqueueMsg(msg);
    if (runAsyncCommand(&CacheConsumer::readIndexCallback, command, id) != REDIS_OK) {
        disposeId(id);
    }
}

void CacheConsumer::requestKeys(const QStringList &keys, const Radapter::WorkerMsg &msg)
{
    if (keys.isEmpty()) {
        finishKeys( JsonDict{}, msg);
        return;
    }

    auto command = RedisQueryFormatter::toMultipleGetCommand(keys);
    m_requestedKeysBuffer.enqueue(keys);
    auto id = enqueueMsg(msg);
    if (runAsyncCommand(&CacheConsumer::readKeysCallback, command, id) != REDIS_OK) {
        disposeId(id);
    }
}

void CacheConsumer::readIndexCallback(redisReply *reply, void *msgId)
{
    auto msg = dequeueMsg(msgId);
    if (isNullReply(reply)) {
        return;
    }
    if (reply->elements == 0) {
        reDebug() << metaInfo().c_str() << "Empty index.";
        finishIndex( QVariantList{}, msg);
        return;
    }

    auto indexedKeys = QVariantList{};
    reDebug() << metaInfo().c_str() << "Keys added to index:" << reply->elements;
    for (quint16 i = 0; i < reply->elements; i++) {
        auto keyItem = QString(reply->element[i]->str);
        if (!keyItem.isEmpty()) {
            indexedKeys.append(keyItem);
        }
    }
    finishIndex(indexedKeys, msg);
}

void CacheConsumer::readKeysCallback(redisReply *reply, void *msgId)
{
    auto msg = dequeueMsg(msgId);
    if (isNullReply(reply)) {
        return;
    }
    auto foundEntries = QVariantList{};
    quint16 keysMatched = 0u;
    for (quint16 i = 0; i < reply->elements; i++) {
        auto entryItem = QString(reply->element[i]->str);
        if (!entryItem.isEmpty()) {
            keysMatched++;
        }
        foundEntries.append(entryItem);
    }
    reDebug() << metaInfo().c_str() << "Key entries found:" << keysMatched;
    auto resultJson = mergeWithKeys(foundEntries);
    finishKeys(resultJson, msg);
}

void CacheConsumer::finishIndex(const QVariantList &json, const Radapter::WorkerMsg &msg)
{
    auto reply = prepareReply(msg, Radapter::WorkerMsg::ReplyOk);
    for (auto &jsonDict : json) {
        reply.setJson(jsonDict.toMap()); // id stays the same, command issuer can check id to receive reply
        emit sendMsg(reply);
    }
}

void CacheConsumer::finishKeys(const JsonDict &json, const Radapter::WorkerMsg &msg)
{
    auto reply = prepareReply(msg, Radapter::WorkerMsg::ReplyOk);
    reply.setJson(json);
    emit sendMsg(reply);
}

 JsonDict CacheConsumer::mergeWithKeys(const QVariantList &entries)
{
    auto requestedKeys = m_requestedKeysBuffer.dequeue();
    if (requestedKeys.isEmpty()) {
        return JsonDict{};
    }
    auto jsonDict = JsonDict{};
    for (quint16 i = 0; i < entries.count(); i++) {
        auto entryValue = entries.at(i).toString();
        if (!entryValue.isEmpty()) {
            auto key = requestedKeys.at(i);
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
    }
    if (requestedKeys.isValid()) {
        requestKeys(requestedKeys.toStringList(), msg);
    }
}
