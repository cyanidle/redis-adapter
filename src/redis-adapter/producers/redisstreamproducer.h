#ifndef REDISSTREAMPRODUCER_H
#define REDISSTREAMPRODUCER_H

#include <QObject>
#include <QQueue>
#include <QTimer>
#include "redis-adapter/connectors/redisconnector.h"

namespace Redis {
class RADAPTER_SHARED_SRC RedisStreamProducer;
}

class Redis::RedisStreamProducer : public RedisConnector
{
    Q_OBJECT
public:
    explicit RedisStreamProducer(const QString &host,
                                 const quint16 port,
                                 const QString &streamKey,
                                 const Radapter::WorkerSettings &settings,
                                 const qint32 streamSize = 0u);
    ~RedisStreamProducer() override;
    Radapter::WorkerMsg::SenderType workerType() const override {return Radapter::WorkerMsg::TypeRedisStreamProducer;}

    QString streamKey() const;
    quint32 streamSize() const;

public slots:
    void run() override;
    void onMsg(const Radapter::WorkerMsg &msg) override;

private slots:
    void tryTrim();

private:
    // Replies
    void writeDone(const QString &newEntryId, quint64 msgId);

    static void writeCallback(redisAsyncContext *context, void *replyPtr, void *args);
    static void trimCallback(redisAsyncContext *context, void *replyPtr, void *sender);
    QString id() const override;

    QTimer* m_trimTimer;
    quint16 m_addCounter;
    QString m_streamKey;
    quint32 m_streamSize;
};

#endif // REDISSTREAMPRODUCER_H
