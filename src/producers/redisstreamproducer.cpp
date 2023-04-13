#include "redisstreamproducer.h"
#include <QVariantMap>
#include <QDebug>
#include "broker/workers/private/workermsg.h"
#include "radapterlogging.h"
#include "formatting/redis/redisstreamqueries.h"

#define TRIM_TIMEOUT_MS     60000
#define ADDS_COUNT_TO_TRIM  1000u

using namespace Redis;

StreamProducer::StreamProducer(const Settings::RedisStreamProducer &config, QThread *thread)
    : Connector(config, thread),
      m_trimTimer(nullptr),
      m_addCounter{},
      m_streamKey(config.stream_key),
      m_streamSize(config.stream_size)
{
    m_trimTimer = new QTimer(this);
    m_trimTimer->setInterval(TRIM_TIMEOUT_MS);
    m_trimTimer->setSingleShot(false);
    m_trimTimer->callOnTimeout(this, &StreamProducer::tryTrim);
    m_trimTimer->start();
}

StreamProducer::~StreamProducer()
{
    if (m_trimTimer) {
        m_trimTimer->stop();
        m_trimTimer->deleteLater();
    }
}

QString StreamProducer::streamKey() const
{
    return m_streamKey;
}

quint32 StreamProducer::streamSize() const
{
    return m_streamSize;
}

void StreamProducer::onMsg(const Radapter::WorkerMsg &msg)
{
    if (msg.isEmpty()) {
        return;
    }
    auto command = addToStream(m_streamKey, msg, streamSize());
    if (!command.isEmpty()) {
        if (runAsyncCommand(&StreamProducer::writeCallback, command) != REDIS_OK) {
            auto reply = prepareReply(msg, new Radapter::ReplyFail);
            emit sendMsg(reply);
        }
        m_addCounter++;
    }
}

void StreamProducer::tryTrim()
{
    if (m_addCounter >= ADDS_COUNT_TO_TRIM) {
        auto command = trimStream(m_streamKey, streamSize());
        if (command.isEmpty()) {
            return;
        }
        runAsyncCommand(&StreamProducer::trimCallback, command);
        m_addCounter = 0u;
    }
}

void StreamProducer::writeCallback(redisReply *reply)
{

}

void StreamProducer::trimCallback(redisReply *reply)
{

}

