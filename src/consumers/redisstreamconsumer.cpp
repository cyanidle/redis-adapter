#include "redisstreamconsumer.h"
#include <QDateTime>
#include "formatting/redis/redisstreamqueries.h"
#include "localstorage.h"
#include "radapterlogging.h"

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
    auto startId = lastReadId();
    auto readCommand = readStream(m_streamKey, ENTRIES_PER_READ, BLOCK_TIMEOUT_MS, startId);
    runAsyncCommand(&StreamConsumer::readCallback, readCommand);
}

void StreamConsumer::readCallback(redisReply *reply)
{
    auto parsed = parseReply(reply);

}

void StreamConsumer::setLastReadId(const QString &lastId)
{
    if (m_startMode == Settings::RedisStreamConsumer::StartPersistentId) {
        LocalStorage::instance()->setLastStreamId(m_streamKey, lastId);
    } else {
        m_lastStreamId = lastId;
    }
}

