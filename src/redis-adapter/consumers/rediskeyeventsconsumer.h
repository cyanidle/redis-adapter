#ifndef REDISKEYEVENTSCONSUMER_H
#define REDISKEYEVENTSCONSUMER_H

#include <QObject>
#include "rediscacheconsumer.h"

namespace Redis {
class RADAPTER_SHARED_SRC KeyEventsConsumer;
}

class Redis::KeyEventsConsumer : public Connector
{
    Q_OBJECT
public:
    explicit KeyEventsConsumer(const QString &host,
                               const quint16 port,
                               const QStringList &keyEvents,
                               const Radapter::WorkerSettings &settings, QThread *thread);
public slots:
    void run() override;

private slots:
    void subscribeToKeyEvents();
    void unsubscribe();
private:
    //General Data out
    void finishMessageRead(const QVariantList &jsons);
    void eventReceived(const QVariantList &jsonMessage);
    //Redis Callbacks
    void readMessageCallback(redisAsyncContext *context, redisReply *reply);

    void subscribeToKeyEventsImpl(const QStringList &eventTypes);

   QStringList m_keyEventNotifications;
    bool m_isSubscribed;
};

#endif // REDISKEYEVENTSCONSUMER_H
