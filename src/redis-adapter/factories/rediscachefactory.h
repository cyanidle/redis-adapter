#ifndef REDISCACHEFACTORY_H
#define REDISCACHEFACTORY_H

#include <QObject>
#include <QThread>
#include "redis-adapter/consumers/rediscacheconsumer.h"
#include "redis-adapter/producers/rediscacheproducer.h"
#include "redis-adapter/producers/producerfilter.h"
#include "redis-adapter/settings/redissettings.h"
#include "radapter-broker/factorybase.h"

namespace Redis {
class RADAPTER_SHARED_SRC CacheFactory;
}

class Redis::CacheFactory : public Radapter::FactoryBase
{
    Q_OBJECT
public:
    explicit CacheFactory(const QList<Settings::RedisCache> &cachesList, QObject *parent = nullptr);

    Radapter::WorkersList getWorkers() const;
    void run() override;
    int initSettings() override {return 0;}
    int initWorkers() override;

signals:

private:
    Radapter::WorkersList m_workersPool;
    QList<Settings::RedisCache> m_cachesInfoList;
};

#endif // REDISCACHEFACTORY_H
