#include "rediscacheconsumer.h"
#include "formatting/redis/rediscachequeries.h"
#include "radapterlogging.h"
#include <QTimer>
#include "settings/redissettings.h"
#include "commands/rediscommands.h"
#include "include/redisconstants.h"
#include "templates/algorithms.hpp"

using namespace Redis;
using namespace Cache;
using namespace Radapter;

struct CacheConsumer::Private {
    QString objectKey;
    Radapter::ContextManager<CacheContext> manager;
    QTimer *objectRead;
    bool subscribed{false};
};

CacheConsumer::CacheConsumer(const Settings::RedisCacheConsumer &config, QThread *thread) :
    Connector(config, thread),
    d(new Private{config.object_hash_key, {}, nullptr})
{
    if (config.object_hash_key.wasUpdated()) {
        if (config.use_polling) {
            d->objectRead = new QTimer(this);
            d->objectRead->callOnTimeout(this, &CacheConsumer::requestObjectSimple);
            d->objectRead->setInterval(config.update_rate);
        } else {
            connect(this, &Connector::connected, this, &CacheConsumer::subscribe);
            connect(this, &Connector::disconnected, this, [this]{
                d->subscribed = false;
            });
        }
    }
    connect(this, &Connector::disconnected, this, &CacheConsumer::onDisconnect);
}

CacheConsumer::~CacheConsumer()
{
    delete d;
}

void CacheConsumer::subscribe()
{
    if (d->subscribed) return;
    runAsyncCommand(&CacheConsumer::onEvent, subscribeTo({d->objectKey}));
}

void CacheConsumer::onEvent(redisReply *)
{
    requestObjectSimple();
}

void CacheConsumer::onDisconnect()
{
    d->manager.forEach(&CacheContext::fail, "Disconnected");
    d->manager.clearAll();
}

void CacheConsumer::requestObject(const QString &objectKey, CtxHandle handle)
{
    auto command = QStringLiteral("HGETALL ") + objectKey;
    if (runAsyncCommand(&CacheConsumer::readObjectCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("Object request fail");
    }
}

void CacheConsumer::readObjectCallback(redisReply *reply, CtxHandle handle)
{
    auto result = parseHashReply(reply);
    workerInfo(this) << ": Found object fields: " << result.count();
    if (result.isEmpty()) {
        getCtx(handle).fail("Empty object hash!");
    }
    getCtx(handle).reply(ReplyJson(JsonDict{result}));
}

void CacheConsumer::requestKeys(const QStringList &keys, CtxHandle handle)
{
    auto command = QStringLiteral("MGET ") + keys.join(" ");
    if (runAsyncCommand(&CacheConsumer::readKeysCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("MGET Error");
    }
}

void CacheConsumer::readKeysCallback(redisReply *reply, CtxHandle handle)
{
    auto foundEntries = parseReply(reply).toStringList();
    workerInfo(this) << "Key entries found:" << foundEntries.size();
    getCtx(handle).reply(ReadKeys::WantedReply(foundEntries));
}

void CacheConsumer::requestKey(const QString &key, CtxHandle handle)
{
    auto command = QStringLiteral("GET ") + key;
    if (runAsyncCommand(&CacheConsumer::readKeyCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("GET Error");
    }
}

void CacheConsumer::readKeyCallback(redisReply *replyPtr, CtxHandle handle)
{
    auto foundKey = parseReply(replyPtr).toString();
    workerInfo(this) << "Key found:" << foundKey;
    getCtx(handle).reply(ReadKey::WantedReply(foundKey));
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
    workerInfo(this) << "Hash entries found:" << result.size();
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
    workerInfo(this) << "Set entries found:" << parsed.size();
    getCtx(handle).reply(ReadSet::WantedReply(parsed));
}

void CacheConsumer::requestObjectSimple()
{
    if (!isConnected()) return;
    auto command = QStringLiteral("HGETALL ") + d->objectKey;
    auto handle = d->manager.create<SimpleMsgContext>(this);
    if (runAsyncCommand(&CacheConsumer::readObjectCallback, command, handle) != REDIS_OK) {
        getCtx(handle).fail("Object request fail");
    }
}

void CacheConsumer::handleCommand(const Radapter::Command *command, CtxHandle handle)
{
    if (command->is<ReadKeys>()) {
        requestKeys(command->as<ReadKeys>()->keys(), handle);
    } else if (command->is<ReadKey>()) {
        requestKey(command->as<ReadKey>()->key(), handle);
    } else if (command->is<ReadSet>()) {
        requestSet(command->as<ReadSet>()->set(), handle);
    } else if (command->is<ReadHash>()) {
        requestHash(command->as<ReadHash>()->hash(), handle);
    } else if (command->is<ReadObject>()) {
        requestObject(command->as<ReadObject>()->key(), handle);
    } else {
        getCtx(handle).fail(QStringLiteral("Command type unsupported: ") + command->metaObject()->className());
    }
}

CacheContext &CacheConsumer::getCtx(CtxHandle handle)
{
    return d->manager.get(handle);
}

void CacheConsumer::onCommand(const WorkerMsg &msg)
{
    if (msg.command()->is<CommandPack>()) {
        handleCommand(msg.command()->as<CommandPack>()->first(), d->manager.create<PackContext>(msg, this));
    } else if (msg.command()->is<CommandTriggerRead>()) {
        requestObject(d->objectKey, d->manager.create<SimpleMsgContext>(this));
    } else {
        handleCommand(msg.command(), d->manager.create<SimpleContext>(msg, this));
    }
    d->manager.clearDone();
}

void CacheConsumer::onRun()
{
    if (d->objectRead) {
        d->objectRead->start();
    }
    Connector::onRun();
}

