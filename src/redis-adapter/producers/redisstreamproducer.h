#ifndef REDISSTREAMPRODUCER_H
#define REDISSTREAMPRODUCER_H

#include <QObject>
#include <QQueue>
#include <QTimer>
#include "redis-adapter/connectors/redisconnector.h"

namespace Redis {
class RADAPTER_SHARED_SRC StreamProducer;
}

class Redis::StreamProducer : public Connector
{
    Q_OBJECT
public:
    explicit StreamProducer(const QString &host,
                            const quint16 port,
                            const QString &streamKey,
                            const Radapter::WorkerSettings &settings,
                            QThread *thread,
                            const qint32 streamSize = 0u);
    ~StreamProducer() override;

    QString streamKey() const;
    quint32 streamSize() const;

public slots:
    void run() override;
    void onMsg(const Radapter::WorkerMsg &msg) override;

private slots:
    void tryTrim();

private:
    // Replies
    void writeDone(const QString &newEntryId, const Radapter::WorkerMsg &msg);

    void writeCallback(redisReply *replyPtr, void *msgId);
    void trimCallback(redisReply *replyPtr);
    QString id() const override;

    QTimer* m_trimTimer;
    quint16 m_addCounter;
    QString m_streamKey;
    quint32 m_streamSize;
};

#endif // REDISSTREAMPRODUCER_H
