#include "rediscachefactory.h"
#include <radapter-broker/broker.h>

using namespace Redis;

CacheFactory::CacheFactory(const QList<Settings::RedisCache> &cachesList, QObject *parent)
    : FactoryBase(parent),
      m_workersPool{},
      m_cachesInfoList(cachesList)
{
}

int CacheFactory::initWorkers()
{
    if (m_cachesInfoList.isEmpty()) {
        return 0;
    }

    //! \todo fix me (separate creation of cache consumer and cache producer)
    for (auto &cacheInfo : m_cachesInfoList) {
        auto thread = new QThread{};
        QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        auto settings = Radapter::WorkerSettings();
        settings.thread = new QThread(this);
        settings.name = cacheInfo.name;
        settings.producers = cacheInfo.producers;
        settings.consumers = cacheInfo.consumers;
        settings.isDebug = cacheInfo.debug;
        if (cacheInfo.mode == Settings::RedisCache::Producer) {
            auto worker = new CacheProducer(cacheInfo.target_server.server_host,
                                            cacheInfo.target_server.server_port,
                                            cacheInfo.db_index,
                                            cacheInfo.index_key,
                                            settings);
            QObject::connect(thread, &QThread::started, worker, &CacheProducer::run);
            QObject::connect(thread, &QThread::finished, worker, &CacheProducer::deleteLater);
            m_workersPool.append(worker);
            Radapter::Broker::instance()->registerProxy(worker->createProxy());
        } else if (cacheInfo.mode == Settings::RedisCache::Consumer) {
            auto worker = new CacheConsumer(cacheInfo.target_server.server_host,
                                            cacheInfo.target_server.server_port,
                                            cacheInfo.db_index,
                                            cacheInfo.index_key,
                                            settings);
            QObject::connect(thread, &QThread::started, worker, &CacheConsumer::run);
            QObject::connect(thread, &QThread::finished, worker, &CacheConsumer::deleteLater);
            m_workersPool.append(worker);
            Radapter::Broker::instance()->registerProxy(worker->createProxy());
        } else {
            reDebug() << "Cache worker mode not set for: " << cacheInfo.name;
            return 1;
        }
    }
    return 0;
}

void CacheFactory::run()
{
    for (auto &worker : m_workersPool) {
        worker->thread()->start();
    }
}
