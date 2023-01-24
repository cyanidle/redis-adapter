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

void CacheConsumer::requestIndex(const QString &indexKey, const Radapter::WorkerMsg &msg)
{
    if (indexKey.isEmpty()) {
        requestIndex(m_indexKey, msg);
        return;
    }
    auto command = RedisQueryFormatter::toGetIndexCommand(indexKey);
    auto ptr = new Radapter::WorkerMsg(msg);
    if (runAsyncCommand(&CacheConsumer::readKeysCallback, command, ptr) != REDIS_OK) {
        emit sendMsg(prepareReply(msg, Radapter::WorkerMsg::ReplyFail));
        delete ptr;
    }
}

void CacheConsumer::readIndexCallback(redisReply *reply, Radapter::WorkerMsg *msg)
{
    if (isNullReply(reply)) {
        emit sendMsg(prepareReply(*msg, Radapter::WorkerMsg::ReplyFail, "Null reply"));
        delete msg;
        return;
    }
    if (reply->elements == 0) {
        reDebug() << metaInfo().c_str() << "Empty index.";
        emit sendMsg(prepareReply(*msg, Radapter::WorkerMsg::ReplyFail));
        delete msg;
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
    emit sendMsg(prepareReply(*msg, Radapter::WorkerMsg::ReplyOk, indexedKeys));
    delete msg;
}

void CacheConsumer::requestKeys(const QStringList &keys, const Radapter::WorkerMsg &msg)
{
    auto command = RedisQueryFormatter::toMultipleGetCommand(keys);
    auto ptr = new Radapter::WorkerMsg(msg);
    if (runAsyncCommand(&CacheConsumer::readKeysCallback, command, ptr) != REDIS_OK) {
        emit sendMsg(prepareReply(msg, Radapter::WorkerMsg::ReplyFail));
        delete ptr;
    }
}

void CacheConsumer::readKeysCallback(redisReply *replyPtr, Radapter::WorkerMsg *msg)
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
    auto reply = prepareReply(*msg, Radapter::WorkerMsg::ReplyOk);
    reply.setJson(mergeWithKeys(foundEntries));
    emit sendMsg(reply);
    delete msg;
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
    auto command = msg.commandType();
    auto data = msg.commandData();
    switch (command) {
    case Radapter::WorkerMsg::CommandRequestIndex:
        requestIndex(data.toString(), msg);
        break;
    case Radapter::WorkerMsg::CommandRequestKeys:
        requestKeys(data.toStringList(), msg);
        break;
    default:
        break;
    }
}

