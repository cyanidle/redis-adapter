#ifndef REDISKEYEVENTSCONSUMER_H
#define REDISKEYEVENTSCONSUMER_H

#include <QObject>
#include "connectors/redisconnector.h"
#include "formatting/redis/rediscachequeries.h"

namespace Redis {
class RADAPTER_API KeyEventsConsumer;
}

class Redis::KeyEventsConsumer : public Connector
{
    Q_OBJECT
public:
    explicit KeyEventsConsumer(const Settings::RedisKeyEventSubscriber &config, QThread *thread);
private slots:
    void subscribe();
    void unsubscribe();
private:
    void readMessageCallback(redisReply *reply);

    QStringList m_keyEventNotifications;
    bool m_isSubscribed{false};
};

#endif // REDISKEYEVENTSCONSUMER_H
