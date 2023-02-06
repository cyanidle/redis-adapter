#ifndef REDISSTREAMCONSUMER_H
#define REDISSTREAMCONSUMER_H

#include <QObject>
#include <QTimer>
#include "connectors/redisconnector.h"
#include "settings/redissettings.h"

namespace Redis{

class RADAPTER_SHARED_SRC StreamConsumer : public Connector
{
    Q_OBJECT
public:
    explicit StreamConsumer(const Settings::RedisStreamConsumer &config, QThread *thread);
    QString lastReadId() const;
    const QString &streamKey() const;
private slots:
    void doRead();
private:
    void readCallback(redisReply *replyPtr);
    int toCommandTimeout(int timeoutMsecs) const;
    void setLastReadId(const QString &lastId);

    QString m_streamKey;
    Settings::RedisStreamConsumer::StartMode m_startMode;
    QString m_lastStreamId;
};

}

#endif // REDISSTREAMCONSUMER_H
