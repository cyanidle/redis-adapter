#include "rediscachefactory.h"
#include "radapter-broker/debugging/logginginterceptor.h"
#include <radapter-broker/broker.h>
#include "redis-adapter/radapterlogging.h"

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
        QSet<Radapter::InterceptorBase*> loggers;
        if (cacheInfo.log_jsons) {
            loggers.insert(new Radapter::LoggingInterceptor(*cacheInfo.log_jsons));
        }
        if (cacheInfo.mode == Settings::RedisCache::Producer) {
            auto worker = new CacheProducer(cacheInfo.target_server.server_host,
                                            cacheInfo.target_server.server_port,
                                            cacheInfo.db_index,
                                            cacheInfo.index_key,
                                            cacheInfo.worker,
                                            thread);
            QObject::connect(thread, &QThread::started, worker, &CacheProducer::run);
            QObject::connect(thread, &QThread::finished, worker, &CacheProducer::deleteLater);
            m_workersPool.insert(worker);
            Radapter::Broker::instance()->registerProxy(worker->createProxy(loggers));
        } else if (cacheInfo.mode == Settings::RedisCache::Consumer) {
            auto worker = new CacheConsumer(cacheInfo.target_server.server_host,
                                            cacheInfo.target_server.server_port,
                                            cacheInfo.db_index,
                                            cacheInfo.index_key,
                                            cacheInfo.worker,
                                            thread);
            QObject::connect(thread, &QThread::started, worker, &CacheConsumer::run);
            QObject::connect(thread, &QThread::finished, worker, &CacheConsumer::deleteLater);
            m_workersPool.insert(worker);
            Radapter::Broker::instance()->registerProxy(worker->createProxy(loggers));
        } else {
            reDebug() << "Cache worker mode not set for: " << cacheInfo.worker.name;
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
