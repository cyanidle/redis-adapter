#include "rediscacheconsumer.h"
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "redis-adapter/radapterlogging.h"
#include "radapter-broker/reply.h"
#include "redis-adapter/commands/rediscommands.h"
#include "redis-adapter/include/redismessagekeys.h"

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
    if (runAsyncCommand(&CacheConsumer::readIndexCallback, command, ptr) != REDIS_OK) {
        failMsg(ptr);
    }
}

void CacheConsumer::readIndexCallback(redisReply *reply, WorkerMsg *msg)
{
    if (!isValidReply(reply)) {
        failMsg(msg, "Null redis reply");
        return;
    }
    if (reply->elements == 0) {
        failMsg(msg, "Empty index");
        return;
    }
    auto result = parseReply(reply).toStringList();
    for (auto &entry : result) {
        if (entry.startsWith(REDIS_SET_PREFIX)) {
            //todo
        } else if (entry.startsWith(REDIS_STR_PREFIX)) {
            //todo
        } else if (entry.startsWith(REDIS_HASH_PREFIX)) {
            //todo
        } else {
            //todo
        }
    }
    auto finalReply = prepareReply(*msg, ReadIndex::makeReply(JsonDict{}));
    emit sendMsg(finalReply);
    delete msg;
}

void CacheConsumer::requestKeys(const QStringList &keys, const WorkerMsg &msg)
{
    auto command = RedisQueryFormatter::toMultipleGetCommand(keys);
    auto ptr = new WorkerMsg(msg);
    if (runAsyncCommand(&CacheConsumer::readKeysCallback, command, ptr) != REDIS_OK) {
        failMsg(ptr);
    }
}

void CacheConsumer::readKeysCallback(redisReply *replyPtr, WorkerMsg *msg)
{
    auto readKeys = msg->command()->as<ReadKeys>();
    auto foundEntries = parseReply(replyPtr).toStringList();
    reDebug() << metaInfo() << "Key entries found:" << foundEntries.size();
    auto reply = prepareReply(*msg, readKeys->makeReply(foundEntries));
    reply.setJson(mergeWithKeys(readKeys->keys(), foundEntries));
    emit sendMsg(reply);
    delete msg;
}

void CacheConsumer::requestHash(const QString &hash, const Radapter::WorkerMsg &msg)
{
    auto command = QStringLiteral("HGETALL ") + hash;
    auto ptr = new WorkerMsg(msg);
    if (runAsyncCommand(&CacheConsumer::readHashCallback, command, ptr) != REDIS_OK) {
        failMsg(ptr);
    }
}

void CacheConsumer::readHashCallback(redisReply *replyPtr, Radapter::WorkerMsg *msg)
{
    auto result = parseReply(replyPtr);
    reDebug() << result;
}

void CacheConsumer::failMsg(Radapter::WorkerMsg *msg, const QString &reason)
{
    if (reason != QStringLiteral("Not Given")) {
        reDebug() << metaInfo() << reason;
    }
    emit sendMsg(prepareReply(*msg, new Radapter::ReplyFail(reason)));
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

void CacheConsumer::requestSet(const QString &setKey, const WorkerMsg &msg)
{
    auto command = QStringLiteral("SMEMBERS ") + setKey;
    auto ptr = new WorkerMsg(msg);
    if (runAsyncCommand(&CacheConsumer::readSetCallback, command, ptr) != REDIS_OK) {
        failMsg(ptr);
    }
}

void CacheConsumer::readSetCallback(redisReply *replyPtr, WorkerMsg *msg)
{
    auto cmd = msg->command()->as<ReadSet>();
    auto parsed = parseReply(replyPtr).toStringList();
    if (parsed.isEmpty()) {
        failMsg(msg, "Empty set");
        return;
    }
    auto reply = prepareReply(*msg, cmd->makeReply(parsed));
    emit sendMsg(reply);
    delete msg;
}


void CacheConsumer::onCommand(const WorkerMsg &msg)
{
    handleCommand(msg.command(), msg);
}

void CacheConsumer::handleCommand(const Radapter::Command *command, const Radapter::WorkerMsg &msg)
{
    if (command->is<ReadIndex>()) {
        requestIndex(command->as<ReadIndex>()->index(), msg);
    } else if (command->is<ReadKeys>()) {
        requestKeys(command->as<ReadKeys>()->keys(), msg);
    } else if (command->is<ReadSet>()) {
        requestSet(command->as<ReadSet>()->set(), msg);
    } else if (command->is<CommandPack>()) {
        requestMultiple(command->as<CommandPack>()->commands(), msg);
    } else {
        reError() << metaInfo() << "Command type unsupported: " << command->metaObject()->className();
    }
}

void CacheConsumer::requestMultiple(const QSet<Radapter::Command *> &commands, const Radapter::WorkerMsg &msg)
{
    for (auto &command : commands) {
        handleCommand(command, msg);
    }
}

