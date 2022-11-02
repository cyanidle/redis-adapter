#include "redisstreamconsumer.h"
#include <QDateTime>
#include "redis-adapter/localstorage.h"
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "redis-adapter/formatters/redisstreamentryformatter.h"
#include "redis-adapter/formatters/streamentriesmapformatter.h"
#include "redis-adapter/radapterlogging.h"
#include "redis-adapter/radapterschemas.h"

#define BLOCK_TIMEOUT_MS    30000
#define POLL_TIMEOUT_MS     3000

using namespace Redis;

StreamConsumer::StreamConsumer(const QString &host,
                                         const quint16 port,
                                         const QString &streamKey,
                                         const QString &groupName,
                                         const Settings::RedisConsumerStartMode startFrom,
                                         const Radapter::WorkerSettings &settings)
    : Connector(host, port, 0u, settings),
      m_streamKey(streamKey),
      m_groupName(groupName),
      m_blockingReadTimer(nullptr),
      m_startMode(startFrom),
      m_lastStreamId{}
{
    m_hasPending = areGroupsEnabled();
}

void StreamConsumer::run()
{
    Connector::run();
    disablePingKeepalive();

    auto readInterval = static_cast<qint32>(BLOCK_TIMEOUT_MS * 1.5);
    m_blockingReadTimer = new QTimer(this);
    m_blockingReadTimer->setInterval(readInterval);
    m_blockingReadTimer->setSingleShot(true);
    m_blockingReadTimer->callOnTimeout(this, &StreamConsumer::blockingRead);

    m_readGroupTimer = new QTimer(this);
    m_readGroupTimer->setInterval(POLL_TIMEOUT_MS);
    m_readGroupTimer->setSingleShot(true);
    m_readGroupTimer->callOnTimeout(this, &StreamConsumer::readGroup);

    connect(this, &StreamConsumer::commandsFinished, this, &StreamConsumer::blockingRead);
    connect(this, &StreamConsumer::connected, this, [this](){
        runAsyncCommand(QString("INCR readers:%1").arg(m_streamKey));
    });
    connect(this, &StreamConsumer::connected, this, &StreamConsumer::createGroup);
    connect(this, &StreamConsumer::connected, this, &StreamConsumer::blockingRead);
    connect(this, &StreamConsumer::connected, this, &StreamConsumer::readGroup);
    connect(this, &StreamConsumer::pendingChanged, &StreamConsumer::updatePingKeepalive);
    connect(this, &StreamConsumer::pendingChanged, &StreamConsumer::updateReadTimers);
    connect(this, &StreamConsumer::ackCompleted, &StreamConsumer::readGroup);
}

void StreamConsumer::blockingRead()
{
    auto commandToSelf = prepareCommand<Radapter::RequestJsonSchema>(true);
    auto id = enqueueMsg(commandToSelf);
    blockingReadImpl(id);
}

void StreamConsumer::readGroup()
{
    auto commandToSelf = prepareCommand<Radapter::RequestJsonSchema>(true);
    auto id = enqueueMsg(commandToSelf);
    readGroupImpl(id);
}

void StreamConsumer::blockingReadImpl(quint64 msgId)
{
    if (hasPendingEntries()) {
        return;
    }
    m_blockingReadTimer->stop();
    doRead(msgId);
    m_blockingReadTimer->start();
}

void StreamConsumer::readGroupImpl(quint64 msgId)
{
    if (!hasPendingEntries()
            || m_blockingReadTimer->isActive()
            || m_readGroupTimer->isActive())
    {
        return;
    }
    doRead(msgId);
    m_readGroupTimer->start();
}

void StreamConsumer::acknowledge(const Formatters::Dict &jsonEntries)
{
    if (!hasPendingEntries()
            || !StreamEntriesMapFormatter::isValid(jsonEntries))
    {
        return;
    }
    m_readGroupTimer->stop();
    auto idList = jsonEntries.keys();
    auto ackCommand = RedisQueryFormatter{}.toReadStreamAckCommand(m_streamKey, groupName(), idList);
    runAsyncCommand(ackCallback, ackCommand);
}

QString StreamConsumer::lastReadId() const
{
    auto lastId = m_lastStreamId;
    if (m_startMode == Settings::RedisStartPersistentId) {
        lastId = LocalStorage::instance()->getLastStreamId(m_streamKey);
    }
    if ((lastId.isEmpty() && (m_startMode == Settings::RedisStartFromFirst))
            || hasPendingEntries())
    {
        lastId = "0-0";
    }
    return lastId;
}

QString StreamConsumer::streamKey() const
{
    return m_streamKey;
}

QString StreamConsumer::groupName() const
{
    return m_groupName;
}

bool StreamConsumer::areGroupsEnabled() const
{
    return !m_groupName.isEmpty();
}

bool StreamConsumer::hasPendingEntries() const
{
    return areGroupsEnabled() && m_hasPending;
}

void StreamConsumer::doRead(quint64 msgId)
{
    auto readCommand = QString{};
    auto startId = lastReadId();
    if (areGroupsEnabled()) {
        readCommand = RedisQueryFormatter{}.toReadGroupCommand(m_streamKey, groupName(), workerName(), BLOCK_TIMEOUT_MS, startId);
    } else {
        readCommand = RedisQueryFormatter{}.toReadStreamCommand(m_streamKey, BLOCK_TIMEOUT_MS, startId);
    }
    runAsyncCommand(readCallback, readCommand, msgId);
}

void StreamConsumer::updatePingKeepalive()
{
    if (!areGroupsEnabled()) {
        return;
    }
    if (hasPendingEntries()) {
        enablePingKeepalive();
    } else {
        disablePingKeepalive();
    }
}

void StreamConsumer::updateReadTimers()
{
    if (!areGroupsEnabled()) {
        return;
    }
    if (hasPendingEntries()) {
        m_blockingReadTimer->stop();
    } else {
        m_readGroupTimer->stop();
    }
}

void StreamConsumer::createGroup()
{
    if (!areGroupsEnabled()) {
        return;
    }
    auto command = RedisQueryFormatter{}.toCreateGroupCommand(m_streamKey, groupName());
    runAsyncCommand(createGroupCallback, command);
}

void StreamConsumer::readCallback(redisAsyncContext *context, void *replyPtr, void *args)
{
    auto actual = static_cast<CallbackArgs*>(args);
    auto sender = actual->sender;
    auto msgId = actual->args.toULongLong();
    if (isNullReply(context, replyPtr, sender)) {
        return;
    }
    auto reply = static_cast<redisReply *>(replyPtr);
    auto adapter = static_cast<StreamConsumer *>(sender);
    if (reply->elements == 0) {
        reDebug() << metaInfo(context).c_str() << "No new entries";
        adapter->finishRead(Formatters::Dict{}, msgId);
        return;
    }

    auto rootEntry = reply->element[0];
    reDebug() << metaInfo(context).c_str() << "stream key:" << rootEntry->element[0]->str;
    auto streamEntries = rootEntry->element[1];
    if (streamEntries->elements == 0) {
        adapter->finishRead(Formatters::Dict{}, msgId);
        return;
    }

    reDebug() << metaInfo(context).c_str() << "new stream entries count:" << streamEntries->elements;
    qint32 entriesCount = 0u;
    auto jsonEntries = Formatters::Dict{};
    for (quint16 i = 0; i < streamEntries->elements; i++) {
        auto jsonDict = RedisStreamEntryFormatter(streamEntries->element[i]).toJson();
        if (!jsonDict.isEmpty()) {
            jsonEntries.insert(jsonDict);
        }
        entriesCount += jsonDict.count();
    }
    reDebug() << metaInfo(context).c_str() << "entries processed:" << entriesCount;
    adapter->finishRead(jsonEntries, msgId);
    delete actual;
}

void StreamConsumer::ackCallback(redisAsyncContext *context, void *replyPtr, void *sender)
{
    if (isNullReply(context, replyPtr, sender)) {
        return;
    }
    auto reply = static_cast<redisReply *>(replyPtr);
    reDebug() << metaInfo(context).c_str() << "entries acknowledged:" << toString(reply);
    auto adapter = static_cast<StreamConsumer *>(sender);
    adapter->finishAck();
}

void StreamConsumer::createGroupCallback(redisAsyncContext *context, void *replyPtr, void *sender)
{
    if (isNullReply(context, replyPtr, sender)
            || isEmptyReply(context, replyPtr))
    {
        return;
    }
    auto reply = static_cast<redisReply *>(replyPtr);
    reDebug() << metaInfo(context).c_str() << "create group status:" << toString(reply);
    auto adapter = static_cast<StreamConsumer *>(sender);
    adapter->finishAsyncCommand();
}

QString StreamConsumer::id() const
{
    auto idString = QString("%1 | %2").arg(metaObject()->className(), streamKey());
    return idString;
}

void StreamConsumer::finishRead(const Formatters::Dict &json, quint64 msgId)
{
    if (!json.isEmpty()) {
        auto reply = prepareReply(dequeueMsg(msgId));
        if (reply.brokerFlags != Radapter::WorkerMsg::BrokerBadMsg) {
            reply.setData(json);
            emit sendMsg(reply);
            setLastReadId(json.lastKey());
        }
    }
    bool needAck = !hasPendingEntries() && !json.isEmpty();
    if (needAck) {
        setPending(true);
    }
    bool isDoneAck = hasPendingEntries() && json.isEmpty();
    if (isDoneAck) {
        setPending(false);
        setLastReadId(QString{});
    }
    finishAsyncCommand();
}

void StreamConsumer::finishAck()
{
    ackCompletedImpl();
    finishAsyncCommand();
}

void StreamConsumer::pendingChangedImpl()
{
    emit pendingChanged();
}

void StreamConsumer::ackCompletedImpl()
{
    emit ackCompleted();
}


void StreamConsumer::setLastReadId(const QString &lastId)
{
    if (m_startMode == Settings::RedisStartPersistentId) {
        LocalStorage::instance()->setLastStreamId(m_streamKey, lastId);
    } else {
        m_lastStreamId = lastId;
    }
}

void StreamConsumer::setPending(bool state)
{
    if (!areGroupsEnabled()) {
        return;
    }
    if (m_hasPending != state) {
        m_hasPending = state;
        pendingChangedImpl();
    }
}

void StreamConsumer::onCommand(const Radapter::WorkerMsg &msg)
{
    if (msg.usesScheme<Radapter::RequestJsonSchema>()) {
        if (m_groupName.isEmpty()) {
            readGroupImpl(enqueueMsg(msg));
        } else {
            blockingReadImpl(enqueueMsg(msg));
        }
    } else {
        reWarn() << "Incorrect schema used for 'Command' to 'Redis::StreamConsumer'";
    }
}

void StreamConsumer::onReply(const Radapter::WorkerMsg &msg)
{
    if (msg.sender == this) {
        auto json = msg;
        json.setData(json.first().toMap());
        emit sendMsg(prepareMsg(json.flatten(":"), MsgToConsumers));
    }
}
