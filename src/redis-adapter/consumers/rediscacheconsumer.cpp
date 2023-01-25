#include "rediscacheconsumer.h"
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "redis-adapter/radapterlogging.h"
#include "radapter-broker/reply.h"
#include "redis-adapter/commands/redis/rediscachecommands.h"

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
    auto command = RedisQueryFormatter::toGetIndexCommand(indexKey);
    auto ptr = new Radapter::WorkerMsg(msg);
    if (runAsyncCommand(&CacheConsumer::readKeysCallback, command, ptr) != REDIS_OK) {
        emit sendMsg(prepareReply(msg, new Radapter::ReplyPlain(false)));
        delete ptr;
    }
}

void CacheConsumer::readIndexCallback(redisReply *reply, Radapter::WorkerMsg *msg)
{
    if (isNullReply(reply)) {
        emit sendMsg(prepareReply(*msg, new Radapter::ReplyFail("Null redis reply")));
        delete msg;
        return;
    }
    if (reply->elements == 0) {
        reDebug() << metaInfo().c_str() << "Empty index.";
        emit sendMsg(prepareReply(*msg, new Radapter::ReplyFail("Empty index")));
        delete msg;
        return;
    }

    auto indexedKeys = QStringList{};
    reDebug() << metaInfo().c_str() << "Keys added to index:" << reply->elements;
    for (quint16 i = 0; i < reply->elements; i++) {
        auto keyItem = QString(reply->element[i]->str);
        if (!keyItem.isEmpty()) {
            indexedKeys.append(keyItem);
        }
    }
    auto finalReply = prepareReply(*msg, new Redis::IndexReply(indexedKeys));
    emit sendMsg(finalReply);
    delete msg;
}

void CacheConsumer::requestKeys(const QStringList &keys, const Radapter::WorkerMsg &msg)
{
    auto command = RedisQueryFormatter::toMultipleGetCommand(keys);
    auto ptr = new Radapter::WorkerMsg(msg);
    if (runAsyncCommand(&CacheConsumer::readKeysCallback, command, ptr) != REDIS_OK) {
        emit sendMsg(prepareReply(msg, new Radapter::ReplyPlain(false)));
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
    auto reply = prepareReply(*msg, new Radapter::ReplyOk);
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
    using Radapter::CommandType;
    using namespace Cache;
    auto command = msg.command();
    if (command->is<ReadIndex>()) {
        requestIndex(command->as<ReadIndex>()->index(), msg);
    } else if (command->is<ReadKeys>()) {
        requestKeys(command->as<ReadKeys>()->keys(), msg);
    } else if (command->is<ReadSet>()) {
        requestSet(command->as<ReadSet>()->set(), msg);
    }
}

void CacheConsumer::requestSet(const QString &setKey, const Radapter::WorkerMsg &msg)
{

}

void CacheConsumer::readSetCallback(redisReply *replyPtr, Radapter::WorkerMsg *msg)
{

}
