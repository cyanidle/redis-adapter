#include "redisstreamproducer.h"
#include <QVariantMap>
#include <QDebug>
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "redis-adapter/radapterlogging.h"

#define DEFAULT_STREAM_SIZE 1000000u
#define TRIM_TIMEOUT_MS     60000
#define ADDS_COUNT_TO_TRIM  1000u

using namespace Redis;

RedisStreamProducer::RedisStreamProducer(const QString &host,
                                         const quint16 port,
                                         const QString &streamKey,
                                         const Radapter::WorkerSettings &settings,
                                         const qint32 streamSize)
    : RedisConnector(host, port, 0u, settings),
      m_trimTimer(nullptr),
      m_addCounter{},
      m_streamKey(streamKey)
{
    m_streamSize = streamSize > 0 ? static_cast<quint32>(streamSize) : DEFAULT_STREAM_SIZE;
}

RedisStreamProducer::~RedisStreamProducer()
{
    if (m_trimTimer) {
        m_trimTimer->stop();
        m_trimTimer->deleteLater();
    }
}

QString RedisStreamProducer::streamKey() const
{
    return m_streamKey;
}

quint32 RedisStreamProducer::streamSize() const
{
    return m_streamSize;
}

void RedisStreamProducer::run()
{
    RedisConnector::run();
    m_trimTimer = new QTimer(this);
    m_trimTimer->setInterval(TRIM_TIMEOUT_MS);
    m_trimTimer->setSingleShot(false);
    m_trimTimer->callOnTimeout(this, &RedisStreamProducer::tryTrim);
    m_trimTimer->start();
}

void RedisStreamProducer::onMsg(const Radapter::WorkerMsg &msg)
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
        runAsyncCommand(writeCallback, command, enqueueMsg(msg));
        m_addCounter++;
    }
}

void RedisStreamProducer::tryTrim()
{
    if (m_addCounter >= ADDS_COUNT_TO_TRIM) {
        auto command = RedisQueryFormatter(Formatters::Dict{}).toTrimCommand(m_streamKey, streamSize());
        if (command.isEmpty()) {
            return;
        }
        runAsyncCommand(trimCallback, command);
        m_addCounter = 0u;
    }
}

void RedisStreamProducer::writeCallback(redisAsyncContext *context, void *replyPtr, void *args)
{
    auto cbArgs = static_cast<CallbackArgs*>(args);
    if (isNullReply(context, replyPtr, cbArgs->sender)
            || isEmptyReply(context, replyPtr))
    {
        return;
    }
    auto reply = static_cast<redisReply *>(replyPtr);
    reDebug() << metaInfo(context).c_str() << "Entry added:" << reply->str;
    auto adapter = static_cast<RedisStreamProducer *>(cbArgs->sender);
    adapter->writeDone(reply->str, cbArgs->args.toULongLong());
    adapter->finishAsyncCommand();
    delete cbArgs;
}

void RedisStreamProducer::writeDone(const QString &newEntryId, quint64 msgId)
{
    auto reply = prepareReply(dequeueMsg(msgId));
    reply["data"] = newEntryId;
    emit sendMsg(reply);
}

void RedisStreamProducer::trimCallback(redisAsyncContext *context, void *replyPtr, void *sender)
{
    if (isNullReply(context, replyPtr, sender)) {
        return;
    }
    auto reply = static_cast<redisReply *>(replyPtr);
    reDebug() << metaInfo(context).c_str() << "Entries trimmed:" << reply->integer;
    auto adapter = static_cast<RedisStreamProducer *>(sender);
    adapter->finishAsyncCommand();
}

QString RedisStreamProducer::id() const
{
    auto idString = QString("%1 | %2").arg(metaObject()->className(), streamKey());
    return idString;
}
