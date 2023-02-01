#include "redisstreamconsumer.h"
#include <QDateTime>
#include "redis-adapter/localstorage.h"
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "redis-adapter/formatters/redisstreamentryformatter.h"
#include "redis-adapter/radapterlogging.h"

#define ENTRIES_PER_READ            1000
#define BLOCK_TIMEOUT_MS            30000
#define POLL_TIMEOUT_MS             3000
#define COMMAND_TIMEOUT_DELAY_MS    150

using namespace Redis;

StreamConsumer::StreamConsumer(const Settings::RedisStreamConsumer &config,
                               QThread *thread)
    : Connector(config, thread),
      m_streamKey(config.stream_key),
      m_startMode(config.start_from),
      m_lastStreamId{}
{
    disablePingKeepalive();
    connect(this, &StreamConsumer::commandsFinished, this, &StreamConsumer::doRead);
    connect(this, &StreamConsumer::connected, this, [this](){
        Connector::runAsyncCommand(QString("INCR readers:%1").arg(m_streamKey));
    });
    connect(this, &StreamConsumer::connected, this, &StreamConsumer::doRead);
}

QString StreamConsumer::lastReadId() const
{
    auto lastId = m_lastStreamId;
    if (m_startMode == Settings::RedisStreamConsumer::StartPersistentId) {
        lastId = LocalStorage::instance()->getLastStreamId(m_streamKey);
    }
    if ((lastId.isEmpty() && (m_startMode == Settings::RedisStreamConsumer::StartFromFirst)))
    {
        lastId = "0-0";
    }
    return lastId;
}

const QString &StreamConsumer::streamKey() const
{
    return m_streamKey;
}

void StreamConsumer::doRead()
{
    resetCommandTimeout();
    auto startId = lastReadId();
    auto readCommand = RedisQueryFormatter::toReadStreamCommand(m_streamKey, ENTRIES_PER_READ, BLOCK_TIMEOUT_MS, startId);
    runAsyncCommand(&StreamConsumer::readCallback, readCommand);
}


void StreamConsumer::readCallback(redisReply *reply)
{
    doRead();
    if (!isValidReply(reply)) {
        return;
    }
    if (reply->elements == 0) {
        reDebug() << metaInfo() << "No new entries";
        return;
    }
    auto rootEntry = reply->element[0];
    reDebug() << metaInfo() << "stream key:" << rootEntry->element[0]->str;
    auto streamEntries = rootEntry->element[1];
    reDebug() << metaInfo() << "new stream entries count:" << streamEntries->elements;
    qint32 entriesCount = 0u;
    auto jsonEntries = JsonDict{};
    for (quint16 i = 0; i < streamEntries->elements; i++) {
        auto formatted = RedisStreamEntryFormatter(streamEntries->element[i]);
        auto jsonDict = formatted.toJson();
        if (!jsonDict.isEmpty()) {
            jsonEntries.merge(jsonDict);
        }
        entriesCount += jsonDict.count();
        if (i == streamEntries->elements - 1) {
            setLastReadId(formatted.entryId());
        }
    }
    reDebug() << metaInfo() << "entries processed:" << entriesCount;
    emit sendMsg(prepareMsg(jsonEntries));
}

void StreamConsumer::setLastReadId(const QString &lastId)
{
    if (m_startMode == Settings::RedisStreamConsumer::StartPersistentId) {
        LocalStorage::instance()->setLastStreamId(m_streamKey, lastId);
    } else {
        m_lastStreamId = lastId;
    }
}

