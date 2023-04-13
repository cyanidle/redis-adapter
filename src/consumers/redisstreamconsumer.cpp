#include "redisstreamconsumer.h"
#include <QDateTime>
#include "broker/workers/private/workermsg.h"
#include "formatting/redis/redisstreamentry.h"
#include "formatting/redis/redisstreamqueries.h"
#include "localstorage.h"
#include "jsondict/jsondict.h"
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
      m_lastStreamId{"$"}
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
        auto fromLast = LocalStorage::instance()->getLastStreamId(m_streamKey);
        lastId = fromLast.isEmpty() ? lastId : fromLast;
    } else if (m_startMode == Settings::RedisStreamConsumer::StartFromFirst) {
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
    auto parsed = parseReply(reply).toList();
    if (parsed.isEmpty()) return;
    auto replies = parsed.first().toList().last().toList();
    for (const auto &entry: replies) {
        auto parsedEntry = StreamEntry(entry.toList());
        emit sendBasic(parsedEntry.values);
        setLastReadId(parsedEntry.streamId());
    }
}

void StreamConsumer::setLastReadId(const QString &lastId)
{
    if (m_startMode == Settings::RedisStreamConsumer::StartPersistentId) {
        LocalStorage::instance()->setLastStreamId(m_streamKey, lastId);
    } else {
        m_lastStreamId = lastId;
    }
}

