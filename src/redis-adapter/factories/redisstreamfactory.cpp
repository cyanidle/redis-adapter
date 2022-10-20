#include "redisstreamfactory.h"
#include "radapter-broker/broker.h"
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
        workerSettings.thread = thread;
        workerSettings.name = streamInfo.name;
        QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        if ((streamInfo.mode == Settings::RedisStreamConsumer)
                || (streamInfo.mode == Settings::RedisStreamConsumerGroups))
        {
            auto consumer = new RedisStreamConsumer(streamInfo.source.server_host,
                                                    streamInfo.source.server_port,
                                                    streamInfo.stream_key,
                                                    streamInfo.consumer_group_name,
                                                    streamInfo.start_from,
                                                    workerSettings);
            QObject::connect(thread, &QThread::started, consumer, &RedisStreamConsumer::run);
            QObject::connect(thread, &QThread::finished, consumer, &RedisStreamConsumer::deleteLater);
            Radapter::Broker::instance()->registerProxy(consumer->createProxy());
            m_workersPool.append(consumer);
        }
        if ((streamInfo.mode == Settings::RedisStreamProducer)) {
            auto producer = new RedisStreamProducer(streamInfo.target.server_host,
                                                    streamInfo.target.server_port,
                                                    streamInfo.stream_key,
                                                    workerSettings,
                                                    streamInfo.stream_size);
            QObject::connect(thread, &QThread::started, producer, &RedisStreamProducer::run);
            QObject::connect(thread, &QThread::finished, producer, &RedisStreamProducer::deleteLater);
            Radapter::Broker::instance()->registerProxy(producer->createProxy());
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
