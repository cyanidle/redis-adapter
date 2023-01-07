#ifndef REDISSTREAMFACTORY_H
#define REDISSTREAMFACTORY_H

#include <QObject>
#include <QThread>
#include "redis-adapter/settings/settings.h"
#include "redis-adapter/producers/redisstreamproducer.h"
#include "radapter-broker/factorybase.h"
#include "redis-adapter/consumers/redisstreamconsumer.h"

namespace Redis{
class RADAPTER_SHARED_SRC StreamFactory;
}

class Redis::StreamFactory : public Radapter::FactoryBase
{
    Q_OBJECT
public:
    explicit StreamFactory(const Settings::RedisStream::Map &streamsMap, QObject *parent = nullptr);

    int initWorkers() override;
    void run() override;
signals:

private:
    Radapter::WorkersList m_workersPool;
    Settings::RedisStream::Map m_streamsInfoMap;
};

#endif // REDISSTREAMFACTORY_H
