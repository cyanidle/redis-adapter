#ifndef REDISSTREAMCONSUMER_H
#define REDISSTREAMCONSUMER_H

#include <QObject>
#include <QTimer>
#include "redis-adapter/connectors/redisconnector.h"
#include "json-formatters/formatters/dict.h"
#include "redis-adapter/protocol.h"
#include "radapter-broker/workerbase.h"
#include "redis-adapter/settings/redissettings.h"

class RADAPTER_SHARED_SRC RedisStreamConsumer : public Redis::RedisConnector
{
    Q_OBJECT
public:
    explicit RedisStreamConsumer(const QString &host,
                                 const quint16 port,
                                 const QString &streamKey,
                                 const QString &groupName,
                                 const Settings::RedisConsumerStartMode startFrom,
                                 const Radapter::WorkerSettings &settings);
    Radapter::WorkerMsg::SenderType workerType() const override {return Radapter::WorkerMsg::TypeRedisStreamConsumer;}
    QString lastReadId() const;

    QString streamKey() const;
    QString groupName() const;

    bool areGroupsEnabled() const;
    bool hasPendingEntries() const;
signals:
    void pendingChanged();
    void ackCompleted();

public slots:
    void run() override;
    void onCommand(const Radapter::WorkerMsg &msg) override;
    void onReply(const Radapter::WorkerMsg &msg) override;
    void blockingRead();
    void readGroup();

//    connect(redisWorker.consumer, &RedisStreamConsumer::jsonReady,
//            m_modbusLauncher, &ModbusConnector::onDataReceived,
//            Qt::QueuedConnection);
//    connect(m_modbusLauncher, &ModbusConnector::jsonItemWritten,
//            redisWorker.consumer, &RedisStreamConsumer::readGroup,
//            Qt::QueuedConnection);
//    connect(m_modbusLauncher, &ModbusConnector::allDevicesConnected,
//            redisWorker.consumer, &RedisStreamConsumer::readGroup,
//            Qt::QueuedConnection);
//    connect(m_modbusLauncher, &ModbusConnector::writeJsonDone,
//            redisWorker.consumer, &RedisStreamConsumer::acknowledge,
//            Qt::QueuedConnection);

private slots:
    void doRead(quint64 msgId);
    void updatePingKeepalive();
    void updateReadTimers();
    void createGroup();
private:
    // Commands
    void blockingReadImpl(quint64 msgId);
    void readGroupImpl(quint64 msgId);
    // Replies
    void acknowledge(const Formatters::Dict &jsonEntries);
    void pendingChangedImpl();
    void ackCompletedImpl();


    static void readCallback(redisAsyncContext *context, void *replyPtr, void *args);
    static void ackCallback(redisAsyncContext *context, void *replyPtr, void *sender);
    static void createGroupCallback(redisAsyncContext *context, void *replyPtr, void *sender);
    QString id() const override;

    void finishRead(const Formatters::Dict &json, quint64 msgId);
    void finishAck();
    void setLastReadId(const QString &lastId);
    void setPending(bool state);

    QString m_streamKey;
    QString m_groupName;
    QTimer* m_blockingReadTimer;
    QTimer* m_readGroupTimer;
    Settings::RedisConsumerStartMode m_startMode;
    QString m_lastStreamId;
    bool m_hasPending;
    Radapter::Protocol* m_proto;
};

#endif // REDISSTREAMCONSUMER_H
