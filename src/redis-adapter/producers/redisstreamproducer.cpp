#include "redisstreamproducer.h"
#include <QVariantMap>
#include <QDebug>
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "redis-adapter/radapterlogging.h"

#define DEFAULT_STREAM_SIZE 1000000u
#define TRIM_TIMEOUT_MS     60000
#define ADDS_COUNT_TO_TRIM  1000u

using namespace Redis;

StreamProducer::StreamProducer(const QString &host,
                               const quint16 port,
                               const QString &streamKey,
                               const Radapter::WorkerSettings &settings,
                               QThread *thread,
                               const qint32 streamSize)
    : Connector(host, port, 0u, settings, thread),
      m_trimTimer(nullptr),
      m_addCounter{},
      m_streamKey(streamKey)
{
    m_streamSize = streamSize > 0 ? static_cast<quint32>(streamSize) : DEFAULT_STREAM_SIZE;
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

void StreamProducer::run()
{
    Connector::run();
    m_trimTimer = new QTimer(this);
    m_trimTimer->setInterval(TRIM_TIMEOUT_MS);
    m_trimTimer->setSingleShot(false);
    m_trimTimer->callOnTimeout(this, &StreamProducer::tryTrim);
    m_trimTimer->start();
}

void StreamProducer::onMsg(const Radapter::WorkerMsg &msg)
{
    if (msg.isEmpty()) {
        return;
    }

    if (!isConnected()) {
        run();
    }
    auto json = msg.data();
    auto command = RedisQueryFormatter(json).toAddStreamCommand(m_streamKey, streamSize());
    if (!command.isEmpty()) {
        auto id = enqueueMsg(msg);
        if (runAsyncCommand(&StreamProducer::writeCallback, command, id) != REDIS_OK) {
            disposeId(id);
            auto reply = prepareReply(msg, Radapter::WorkerMsg::ReplyFail);
            emit sendMsg(reply);
        }
        m_addCounter++;
    }
}

void StreamProducer::tryTrim()
{
    if (m_addCounter >= ADDS_COUNT_TO_TRIM) {
        auto command = RedisQueryFormatter( JsonDict{}).toTrimCommand(m_streamKey, streamSize());
        if (command.isEmpty()) {
            return;
        }
        runAsyncCommand(&StreamProducer::trimCallback, command);
        m_addCounter = 0u;
    }
}

void StreamProducer::writeCallback(redisAsyncContext *context, redisReply *reply, void *msgId)
{
    auto msg = dequeueMsg(msgId);
    if (isNullReply(context, reply)
            || isEmptyReply(context, reply))
    {
        return;
    }
    reDebug() << metaInfo(context).c_str() << "Entry added:" << reply->str;
    writeDone(reply->str, msg);
}

void StreamProducer::writeDone(const QString &newEntryId, const Radapter::WorkerMsg &msg)
{
    auto reply = prepareReply(msg, Radapter::WorkerMsg::ReplyOk, newEntryId);
    emit sendMsg(reply);
}

void StreamProducer::trimCallback(redisAsyncContext *context, redisReply *reply)
{
    if (isNullReply(context, reply)) {
        return;
    }
    reDebug() << metaInfo(context).c_str() << "Entries trimmed:" << reply->integer;
}

QString StreamProducer::id() const
{
    auto idString = QString("%1 | %2").arg(metaObject()->className(), streamKey());
    return idString;
}
