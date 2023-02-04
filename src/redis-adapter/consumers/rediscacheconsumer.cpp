#include "rediscacheconsumer.h"
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "redis-adapter/radapterlogging.h"
#include "radapter-broker/reply.h"
#include "redis-adapter/commands/rediscommands.h"
#include "redis-adapter/include/redismessagekeys.h"
#include "templates/algorithms.hpp"

using namespace Redis;
using namespace Cache;
using namespace Radapter;

QRegExp CacheConsumer::m_firstIsIntChecker{"^[0-9]*:.*$"};
QRegExp CacheConsumer::m_intChecker{"^[0-9]*$"};

CacheConsumer::CacheConsumer(const Settings::RedisCacheConsumer &config, QThread *thread) :
    Connector(config, thread),
    m_indexKey(config.object_hash_key)
{
}

void CacheConsumer::requestObject(const QString &objectKey, CtxHandle handle)
{
    auto command = QStringLiteral("HGETALL ") + objectKey;
    if (runAsyncCommand(&CacheConsumer::readObjectCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail(metaInfo());
    }
}

JsonDict CacheConsumer::parseNestedArrays(const JsonDict &target) {
    JsonDict result;
    bool hadInt = false;
    for (auto &iter : target) {
        auto branchHadInt = false;
        auto key = iter.key();
        for (auto subkey : enumerate(key)) {
            bool isInt = false;
            subkey.value.toInt(&isInt);
            if (!isInt) {
                continue;
            }
            hadInt = branchHadInt = true;
            auto currentKeyToArray = QStringList(key.begin(), key.begin() + subkey.count);
            auto &currentValue = result[currentKeyToArray];
            auto currentArray = QVariantList{};
            const auto &mapInside = target[currentKeyToArray].toMap();
            for (auto iter = mapInside.constBegin(); iter != mapInside.constEnd(); ++iter) {
                bool isSubInt = false;
                auto arrayInd = iter.key().toInt(&isSubInt);
                if (isSubInt && arrayInd > -1) {
                    while (currentArray.size() <= arrayInd) {
                        currentArray.append(QVariant{});
                    }
                    currentArray[arrayInd] = iter.value();
                } else {
                    reWarn() << "Object nested array collision! (Or index < 0)";
                }
            }
            for (auto &val : currentArray) {
                if (val.type() == QVariant::Map) {
                    auto temp = parseNestedArrays(*reinterpret_cast<const JsonDict*>(val.data())).toVariant();
                    val = std::move(temp);
                }
            }
            currentValue.setValue(currentArray);
        }
        if (!branchHadInt) {
            auto keyToInsert = iter.key();
            auto valToInsert = iter.value();
            result.insert(keyToInsert, valToInsert);
        }
    }
    if (hadInt) {
        result = parseNestedArrays(result);
    }
    return result;
}

void CacheConsumer::readObjectCallback(redisReply *reply, CtxHandle handle)
{
    auto result = parseHashReply(reply);
    if (result.isEmpty()) {
        getCtx(handle).fail("Empty object hash!");
    }
    JsonDict foundJson;
    for (auto iter = result.begin(); iter != result.end(); ++iter) {
        bool isFirstInt = !iter.key().indexOf(m_firstIsIntChecker);
        if (isFirstInt) {
            reWarn() << "Number is first in index hash";
            continue;
        }
        auto toMerge = JsonDict();
        toMerge.insert(iter.key().split(':'), iter.value());
        foundJson.merge(toMerge);
    }
    foundJson = parseNestedArrays(foundJson);
    getCtx(handle).reply(ReplyJson(foundJson));
}

void CacheConsumer::requestKeys(const QStringList &keys, CtxHandle handle)
{
    auto command = QStringLiteral("MGET ") + keys.join(" ");
    if (runAsyncCommand(&CacheConsumer::readKeysCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("MGET Error");
    }
}

void CacheConsumer::readKeysCallback(redisReply *replyPtr, CtxHandle handle)
{
    auto foundEntries = parseReply(replyPtr).toStringList();
    reDebug() << metaInfo() << "Key entries found:" << foundEntries.size();
    getCtx(handle).reply(ReadKeys::WantedReply(foundEntries));
}

void CacheConsumer::requestKey(const QString &key, CtxHandle handle)
{
    auto command = QStringLiteral("GET ") + key;
    if (runAsyncCommand(&CacheConsumer::readKeyCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("MGET Error");
    }
}

void CacheConsumer::readKeyCallback(redisReply *replyPtr, CtxHandle handle)
{
    auto foundEntries = parseReply(replyPtr).toString();
    getCtx(handle).reply(ReadKey::WantedReply(foundEntries));
}
void CacheConsumer::requestHash(const QString &hash, CtxHandle handle)
{
    auto command = QStringLiteral("HGETALL ") + hash;
    if (runAsyncCommand(&CacheConsumer::readHashCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("HGETALL Error");
    }
}

void CacheConsumer::readHashCallback(redisReply *replyPtr, CtxHandle handle)
{
    auto result = parseHashReply(replyPtr);
    reDebug() << metaInfo() << "Hash entries found:" << result.size();
    getCtx(handle).reply(ReplyHash(result));
}

void CacheConsumer::requestSet(const QString &setKey, CtxHandle handle)
{
    auto command = QStringLiteral("SMEMBERS ") + setKey;
    if (runAsyncCommand(&CacheConsumer::readSetCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("SMEMBERS Error");
    }
}

void CacheConsumer::readSetCallback(redisReply *replyPtr, CtxHandle handle)
{
    auto parsed = parseReply(replyPtr).toStringList();
    reDebug() << metaInfo() << "Set entries found:" << parsed.size();
    getCtx(handle).reply(ReadSet::WantedReply(parsed));
}

void CacheConsumer::handleCommand(const Radapter::Command *command, CtxHandle handle)
{
    if (command->is<ReadKeys>()) {
        requestKeys(command->as<ReadKeys>()->keys(), handle);
    } else if (command->is<ReadKey>()) {
        requestKey(command->as<ReadKey>()->key(), handle);
    } else if (command->is<ReadSet>()) {
        requestSet(command->as<ReadSet>()->set(), handle);
    }else if (command->is<ReadHash>()) {
        requestHash(command->as<ReadHash>()->hash(), handle);
    } else {
        getCtx(handle).fail(QStringLiteral("Command type unsupported: ") + command->metaObject()->className());
    }
}

void CacheConsumer::requestMultiple(const CommandPack* pack, CtxHandle handle)
{
    if (any_of(pack->commands(), &Command::is<ReadObject>)) {
        auto msgCopy = getCtx(handle).msg();
        auto reply = prepareReply(msgCopy, new ReplyFail("CommandPack Contains ReadObject!"));
        emit sendMsg(reply);
        getCtx(handle).setDone();
    }
    if (!pack->commands().size()) {
        getCtx(handle).fail("Empty command Pack");
        getCtx(handle).setDone();
        return;
    }
    handleCommand(pack->commands().first().data(), handle);
}

CacheContext &CacheConsumer::getCtx(CtxHandle handle)
{
    return m_manager.get(handle);
}

void CacheConsumer::onCommand(const WorkerMsg &msg)
{
    if (msg.command()->is<ReadObject>()) {
        requestObject(msg.command()->as<ReadObject>()->key(), m_manager.create<ObjectContext>(msg, this));
    } else if (msg.command()->is<CommandPack>()) {
        requestMultiple(msg.command()->as<CommandPack>(), m_manager.create<PackContext>(msg, this));
    } else {
        handleCommand(msg.command(), m_manager.create<SimpleContext>(msg, this));
    }
    m_manager.clearBasedOn(&CacheContext::isDone);
}

