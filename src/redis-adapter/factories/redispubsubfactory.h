#ifndef REDISPUBSUBFACTORY_H
#define REDISPUBSUBFACTORY_H

#include <QObject>
#include <QThread>
#include "redis-adapter/settings/redissettings.h"
#include "redis-adapter/consumers/rediskeyeventsconsumer.h"
#include <radapter-broker/factorybase.h>

namespace Redis {
class RADAPTER_SHARED_SRC PubSubFactory;
}

class Redis::PubSubFactory : public Radapter::FactoryBase
{
    Q_OBJECT
public:
    explicit PubSubFactory(const QList<Settings::RedisKeyEventSubscriber> &subscribers, QObject *parent = nullptr);


    void run() override;
    int initWorkers() override;
    Radapter::WorkersList getWorkers() const;
signals:

private:
    Radapter::WorkersList m_workersPool;
    QList<Settings::RedisKeyEventSubscriber> m_subscribersList;
};

#endif // REDISPUBSUBFACTORY_H
