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
    m_indexKey(indexKey)
{
}

Future<QVariantList> CacheConsumer::requestIndex(const QString &indexKey)
{
    auto future = Future<QVariantList>();
    auto setter = future.createSetter();
    if (!indexKey.isEmpty()) {
        requestIndex(indexKey, setter);
    } else if (!m_indexKey.isEmpty()) {
        return requestIndex(m_indexKey);
    } else {
        setter.setDone();
    }
    return future;
}

Future<JsonDict> CacheConsumer::requestKeys(const QStringList &keys)
{
    auto future = Future<JsonDict>();
    auto setter = future.createSetter();
    if (keys.isEmpty()) {
        setter.setDone();
    }
    return future;
}

void CacheConsumer::requestIndex(const QString &indexKey, FutureSetter<QVariantList> setter)
{
    auto command = RedisQueryFormatter::toGetIndexCommand(indexKey);
    m_indexSetters.append(setter);
    if (runAsyncCommand(&CacheConsumer::readKeysCallback, command, &m_keysSetters.last()) != REDIS_OK) {
        m_keysSetters.last().setDone();
        m_keysSetters.removeLast();
    }
}

void CacheConsumer::readIndexCallback(redisReply *reply, FutureSetter<QVariantList> *setter)
{
    if (isNullReply(reply)) {
        return;
    }
    if (reply->elements == 0) {
        reDebug() << metaInfo().c_str() << "Empty index.";
        setter->setDone();
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
    setter->setResult(indexedKeys);
    setter->setDone();
    m_indexSetters.removeOne(*setter);
}

void CacheConsumer::requestKeys(const QStringList &keys, FutureSetter<JsonDict> setter)
{
    auto command = RedisQueryFormatter::toMultipleGetCommand(keys);
    m_requestedKeysBuffer.enqueue(keys);
    m_keysSetters.append(setter);
    if (runAsyncCommand(&CacheConsumer::readKeysCallback, command, &m_keysSetters.last()) != REDIS_OK) {
        m_keysSetters.last().setDone();
        m_keysSetters.removeLast();
    }
}

void CacheConsumer::readKeysCallback(redisReply *replyPtr, FutureSetter<JsonDict> *setter)
{
    auto foundEntries = QVariantList{};
    quint16 keysMatched = 0u;
    for (quint16 i = 0; i < replyPtr->elements; i++) {
        auto entryItem = QString(replyPtr->element[i]->str);
        if (!entryItem.isEmpty()) {
            keysMatched++;
        }
        foundEntries.append(entryItem);
    }
    reDebug() << metaInfo().c_str() << "Key entries found:" << keysMatched;
    setter->setResult(mergeWithKeys(foundEntries));
    setter->setDone();
    m_keysSetters.removeOne(*setter);
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
        requestIndex(requestedJson.toString());
    }
    if (requestedKeys.isValid()) {
        requestKeys(requestedKeys.toStringList());
    }
}

