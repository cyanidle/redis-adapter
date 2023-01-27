#ifndef REDISSTREAMCONSUMER_H
#define REDISSTREAMCONSUMER_H

#include <QObject>
#include <QTimer>
#include "redis-adapter/connectors/redisconnector.h"
#include "jsondict/jsondict.hpp"
#include "radapter-broker/workerbase.h"
#include "redis-adapter/settings/redissettings.h"

namespace Redis{
    class RADAPTER_SHARED_SRC StreamConsumer;
}

class Redis::StreamConsumer : public Connector
{
    Q_OBJECT
public:
    explicit StreamConsumer(const QString &host,
                                 const quint16 port,
                                 const QString &streamKey,
                                 const QString &groupName,
                                 const Settings::RedisConsumerStartMode startFrom,
                                 const Radapter::WorkerSettings &settings, QThread *thread);
    QString lastReadId() const;

    QString streamKey() const;
    QString groupName() const;

    bool areGroupsEnabled() const;
    bool hasPendingEntries() const;
signals:
    void ackCompleted();
    void pendingChanged(bool state);
public slots:
    void onCommand(const Radapter::WorkerMsg &msg) override;

    void blockingRead();
    void readGroup();
private slots:
    void doRead();
    void updateKeepaliveTimers();
    void updateReadTimers();
    void createGroup();
private:
    // Commands
    void blockingReadCommand();
    void readGroupCommand();
    // Replies
    void acknowledge(const JsonDict &jsonEntries);
    void pendingChangedImpl();
    void ackCompletedImpl();

    void readCallback(redisReply *replyPtr);
    void ackCallback(redisReply *replyPtr);
    void createGroupCallback(redisReply *replyPtr);

    int toCommandTimeout(int timeoutMsecs) const;
    void finishRead(const JsonDict &json, const Radapter::WorkerMsg &msg);
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
};

#endif // REDISSTREAMCONSUMER_H
