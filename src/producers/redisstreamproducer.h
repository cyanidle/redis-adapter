#ifndef REDISSTREAMPRODUCER_H
#define REDISSTREAMPRODUCER_H

#include "connectors/redisconnector.h"
class QTimer;
namespace Settings {
struct RedisStreamProducer;
}
namespace Redis {
class RADAPTER_API StreamProducer;
}

class Redis::StreamProducer : public Connector
{
    Q_OBJECT
public:
    explicit StreamProducer(const Settings::RedisStreamProducer &config, QThread *thread);
    ~StreamProducer() override;

    QString streamKey() const;
    quint32 streamSize() const;

public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;

private slots:
    void tryTrim();

private:
    void writeCallback(redisReply *replyPtr);
    void trimCallback(redisReply *replyPtr);

    QTimer* m_trimTimer;
    quint16 m_addCounter;
    QString m_streamKey;
    quint32 m_streamSize;
};

#endif // REDISSTREAMPRODUCER_H
