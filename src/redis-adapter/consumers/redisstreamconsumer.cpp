#include "redisstreamconsumer.h"
#include <QDateTime>
#include "redis-adapter/localstorage.h"
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "redis-adapter/formatters/redisstreamentryformatter.h"
#include "redis-adapter/formatters/streamentriesmapformatter.h"
#include "redis-adapter/radapterlogging.h"

#define BLOCK_TIMEOUT_MS    30000
#define POLL_TIMEOUT_MS     3000

using namespace Redis;

StreamConsumer::StreamConsumer(const QString &host,
                               const quint16 port,
                               const QString &streamKey,
                               const QString &groupName,
                               const Settings::RedisConsumerStartMode startFrom,
                               const Radapter::WorkerSettings &settings,
                               QThread *thread)
    : Connector(host, port, 0u, settings, thread),
      m_streamKey(streamKey),
      m_groupName(groupName),
      m_blockingReadTimer(nullptr),
      m_startMode(startFrom),
      m_lastStreamId{}
{
    m_hasPending = areGroupsEnabled();
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
        Connector::runAsyncCommand(QString("INCR readers:%1").arg(m_streamKey));
    });
    connect(this, &StreamConsumer::connected, this, &StreamConsumer::createGroup);
    connect(this, &StreamConsumer::connected, this, &StreamConsumer::blockingRead);
    connect(this, &StreamConsumer::connected, this, &StreamConsumer::readGroup);
    connect(this, &StreamConsumer::pendingChanged, &StreamConsumer::updatePingKeepalive);
    connect(this, &StreamConsumer::pendingChanged, &StreamConsumer::updateReadTimers);
    connect(this, &StreamConsumer::ackCompleted, &StreamConsumer::readGroup);
}

void StreamConsumer::blockingReadCommand()
{
    if (hasPendingEntries()) {
        return;
    }
    m_blockingReadTimer->stop();
    doRead();
    m_blockingReadTimer->start();
}


void StreamConsumer::blockingRead()
{
    if (hasPendingEntries()) {
        return;
    }
    m_blockingReadTimer->stop();
    doRead();
    m_blockingReadTimer->start();

}

void StreamConsumer::readGroup()
{
    if (!hasPendingEntries()
        || m_blockingReadTimer->isActive()
        || m_readGroupTimer->isActive())
    {
        return;
    }
    doRead();
    m_readGroupTimer->start();

}

void StreamConsumer::readGroupCommand()
{
    if (!hasPendingEntries()
            || m_blockingReadTimer->isActive()
            || m_readGroupTimer->isActive())
    {
        return;
    }
    doRead();
    m_readGroupTimer->start();
}

void StreamConsumer::acknowledge(const JsonDict &jsonEntries)
{
    if (!hasPendingEntries()
            || !StreamEntriesMapFormatter::isValid(jsonEntries))
    {
        return;
    }
    m_readGroupTimer->stop();
    auto idList = jsonEntries.keysDeep();
    auto ackCommand = RedisQueryFormatter::toReadStreamAckCommand(m_streamKey, groupName(), idList);
    runAsyncCommand(&StreamConsumer::ackCallback, ackCommand);
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

void StreamConsumer::doRead()
{
    auto readCommand = QString{};
    auto startId = lastReadId();
    if (areGroupsEnabled()) {
        readCommand = RedisQueryFormatter::toReadGroupCommand(m_streamKey, groupName(), workerName(), BLOCK_TIMEOUT_MS, startId);
    } else {
        readCommand = RedisQueryFormatter::toReadStreamCommand(m_streamKey, BLOCK_TIMEOUT_MS, startId);
    }
    runAsyncCommand(&StreamConsumer::readCallback, readCommand);
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
    auto command = RedisQueryFormatter::toCreateGroupCommand(m_streamKey, groupName());
    runAsyncCommand(&StreamConsumer::createGroupCallback, command);
}

void StreamConsumer::readCallback(redisReply *reply)
{
    if (isNullReply(reply)) {
        return;
    }
    if (reply->elements == 0) {
        reDebug() << metaInfo().c_str() << "No new entries";
        return;
    }

    auto rootEntry = reply->element[0];
    reDebug() << metaInfo().c_str() << "stream key:" << rootEntry->element[0]->str;
    auto streamEntries = rootEntry->element[1];
    reDebug() << metaInfo().c_str() << "new stream entries count:" << streamEntries->elements;
    qint32 entriesCount = 0u;
    auto jsonEntries = JsonDict{};
    for (quint16 i = 0; i < streamEntries->elements; i++) {
        auto jsonDict = RedisStreamEntryFormatter(streamEntries->element[i]).toJson();
        if (!jsonDict.isEmpty()) {
            jsonEntries.insert(jsonDict);
        }
        entriesCount += jsonDict.count();
    }
    reDebug() << metaInfo().c_str() << "entries processed:" << entriesCount;
}

void StreamConsumer::ackCallback(redisReply *reply)
{
    if (isNullReply(reply)) {
        return;
    }
    reDebug() << metaInfo().c_str() << "entries acknowledged:" << toString(reply);
    finishAck();
}

void StreamConsumer::finishAck()
{
    emit ackCompleted();
}

void StreamConsumer::createGroupCallback(redisReply *reply)
{
    if (isNullReply(reply)
            || isEmptyReply(reply))
    {
        return;
    }
    reDebug() << metaInfo().c_str() << "create group status:" << toString(reply);
}

QString StreamConsumer::id() const
{
    auto idString = QString("%1 | %2").arg(metaObject()->className(), streamKey());
    return idString;
}

void StreamConsumer::finishRead(const JsonDict &json, const Radapter::WorkerMsg &msg)
{
    if (!json.isEmpty()) {
        Radapter::WorkerMsg msgToSend(this, consumers());
        const auto streamIds = json.topKeys();
        if (msg.isCommand()) {
            msgToSend = prepareReply(msg, new Radapter::ReplyOk);
        } else {
            msgToSend = prepareMsg();
        }
        for (const auto& key : streamIds) {
            msgToSend.merge(json[key].toMap());
        }
        emit sendMsg(msgToSend);
        setLastReadId(streamIds.constLast());
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
        emit pendingChanged(state);
    }
}

void StreamConsumer::onCommand(const Radapter::WorkerMsg &msg)
{
    if (msg.command()->is<Radapter::CommandRequestJson>()) {
        if (m_groupName.isEmpty()) {
            readGroupCommand();
        } else {
            blockingReadCommand();
        }
    }
}
