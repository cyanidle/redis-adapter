#ifndef REDISKEYEVENTSCONSUMER_H
#define REDISKEYEVENTSCONSUMER_H

#include <QObject>
#include "rediscacheconsumer.h"

namespace Redis {
class RADAPTER_SHARED_SRC KeyEventsConsumer;
}

class Redis::KeyEventsConsumer : public RedisConnector
{
    Q_OBJECT
public:
    explicit KeyEventsConsumer(const QString &host,
                               const quint16 port,
                               const QStringList &keyEvents,
                               const Radapter::WorkerSettings &settings);

    Radapter::WorkerMsg::SenderType workerType() const override {return Radapter::WorkerMsg::TypeRedisKeyEventsConsumer;}
public slots:
    void run() override;

private slots:
    void subscribeToKeyEvents();
    void unsubscribe();
private:
    //General Data out
    void finishMessageRead(const Formatters::List &jsons);
    void eventReceived(const Formatters::List &jsonMessage);
    //Redis Callbacks
    static void readMessageCallback(redisAsyncContext *context, void *replyPtr, void *sender);

    void subscribeToKeyEventsImpl(const QStringList &eventTypes);

    QStringList m_keyEventNotifications;
    bool m_isSubscribed;
};

#endif // REDISKEYEVENTSCONSUMER_H
