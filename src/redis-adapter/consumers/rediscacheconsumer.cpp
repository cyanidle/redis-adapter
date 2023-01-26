#include "rediscacheconsumer.h"
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "redis-adapter/radapterlogging.h"
#include "radapter-broker/reply.h"
#include "redis-adapter/commands/rediscommands.h"

using namespace Redis;
using namespace Cache;
using namespace Radapter;

CacheConsumer::CacheConsumer(const QString &host,
                             const quint16 port,
                             const quint16 dbIndex,
                             const QString &indexKey,
                             const WorkerSettings &settings,
                             QThread *thread) :
    Connector(host, port, dbIndex, settings, thread),
    m_indexKey(indexKey)
{
}

void CacheConsumer::requestIndex(const QString &indexKey, const WorkerMsg &msg)
{
    auto command = RedisQueryFormatter::toGetIndexCommand(indexKey);
    auto ptr = new WorkerMsg(msg);
    if (runAsyncCommand(&CacheConsumer::readKeysCallback, command, ptr) != REDIS_OK) {
        emit sendMsg(prepareReply(msg, new ReplyFail));
        delete ptr;
    }
}

void CacheConsumer::readIndexCallback(redisReply *reply, WorkerMsg *msg)
{
    if (isNullReply(reply)) {
        emit sendMsg(prepareReply(*msg, new ReplyFail("Null redis reply")));
        delete msg;
        return;
    }
    if (reply->elements == 0) {
        reDebug() << metaInfo().c_str() << "Empty index.";
        emit sendMsg(prepareReply(*msg, new ReplyFail("Empty index")));
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
    auto finalReply = prepareReply(*msg, Cache::ReadIndex::makeReply());
    emit sendMsg(finalReply);
    delete msg;
}

void CacheConsumer::requestKeys(const QStringList &keys, const WorkerMsg &msg)
{
    auto command = RedisQueryFormatter::toMultipleGetCommand(keys);
    auto ptr = new WorkerMsg(msg);
    if (runAsyncCommand(&CacheConsumer::readKeysCallback, command, ptr) != REDIS_OK) {
        emit sendMsg(prepareReply(msg, new ReplyFail));
        delete ptr;
    }
}

void CacheConsumer::readKeysCallback(redisReply *replyPtr, WorkerMsg *msg)
{
    auto foundEntries = QStringList{};
    quint16 keysMatched = 0u;
    for (quint16 i = 0; i < replyPtr->elements; i++) {
        auto entryItem = QString(replyPtr->element[i]->str);
        if (!entryItem.isEmpty()) {
            keysMatched++;
        }
        foundEntries.append(entryItem);
    }
    reDebug() << metaInfo().c_str() << "Key entries found:" << keysMatched;
    auto reply = prepareReply(*msg, ReadKeys::makeReply(foundEntries));
    reply.setJson(mergeWithKeys(msg->command()->as<ReadKeys>()->keys(), foundEntries));
    emit sendMsg(reply);
    delete msg;
}

JsonDict CacheConsumer::mergeWithKeys(const QStringList &keys, const QStringList &entries)
{
    if (keys.isEmpty()) {
        return JsonDict{};
    }
    auto jsonDict = JsonDict{};
    for (quint16 i = 0; i < entries.count(); i++) {
        auto entryValue = entries.at(i);
        if (!entryValue.isEmpty()) {
            auto key = keys.at(i);
            jsonDict.insert(key.split(":"), entryValue);
        }
    }
    return jsonDict;
}

void CacheConsumer::onCommand(const WorkerMsg &msg)
{
    auto command = msg.command();
    if (command->is<ReadIndex>()) {
        requestIndex(command->as<ReadIndex>()->index(), msg);
    } else if (command->is<ReadKeys>()) {
        requestKeys(command->as<ReadKeys>()->keys(), msg);
    } else if (command->is<ReadSet>()) {
        requestSet(command->as<ReadSet>()->set(), msg);
    }
}

void CacheConsumer::requestSet(const QString &setKey, const WorkerMsg &msg)
{

}

void CacheConsumer::readSetCallback(redisReply *replyPtr, WorkerMsg *msg)
{

}
