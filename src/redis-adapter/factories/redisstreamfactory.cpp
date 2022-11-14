#include "redisstreamfactory.h"
#include "radapter-broker/broker.h"
#include "radapter-broker/debugging/logginginterceptor.h"
using namespace Radapter;
using namespace Redis;

StreamFactory::StreamFactory(const Settings::RedisStream::Map &streamsMap, QObject *parent)
    : FactoryBase(parent),
      m_workersPool{},
      m_streamsInfoMap(streamsMap)
{
}

int StreamFactory::initWorkers()
{
    if (m_streamsInfoMap.isEmpty()) {
        return 0;
    }

    const auto streamsInfoList = m_streamsInfoMap.values();
    for (auto &streamInfo : streamsInfoList) {
        auto thread = new QThread(this);
        auto workerSettings = WorkerSettings();
        workerSettings.isDebug = streamInfo.debug;
        workerSettings.thread = thread;
        workerSettings.name = streamInfo.name;
        workerSettings.consumers = streamInfo.consumers;
        workerSettings.producers = streamInfo.producers;
        QList<Radapter::InterceptorBase*> loggers;
        if (streamInfo.log_jsons.use) {
            loggers.append(new Radapter::LoggingInterceptor(streamInfo.log_jsons.asSettings()));
        }
        QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        if ((streamInfo.mode == Settings::RedisStreamConsumer)
                || (streamInfo.mode == Settings::RedisStreamConsumerGroups))
        {
            auto consumer = new StreamConsumer(streamInfo.source.server_host,
                                                    streamInfo.source.server_port,
                                                    streamInfo.stream_key,
                                                    streamInfo.consumer_group_name,
                                                    streamInfo.start_from,
                                                    workerSettings);
            QObject::connect(thread, &QThread::started, consumer, &StreamConsumer::run);
            QObject::connect(thread, &QThread::finished, consumer, &StreamConsumer::deleteLater);
            Radapter::Broker::instance()->registerProxy(consumer->createProxy(loggers));
            m_workersPool.append(consumer);
        }
        if ((streamInfo.mode == Settings::RedisStreamProducer)) {
            auto producer = new StreamProducer(streamInfo.target.server_host,
                                                    streamInfo.target.server_port,
                                                    streamInfo.stream_key,
                                                    workerSettings,
                                                    streamInfo.stream_size);
            QObject::connect(thread, &QThread::started, producer, &StreamProducer::run);
            QObject::connect(thread, &QThread::finished, producer, &StreamProducer::deleteLater);
            Radapter::Broker::instance()->registerProxy(producer->createProxy(loggers));
            m_workersPool.append(producer);
        }
    }
    return 0;
}

void StreamFactory::run()
{
    for (auto &worker : qAsConst(m_workersPool)) {
        worker->thread()->start();
    }
}
