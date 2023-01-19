#include "redispubsubfactory.h"
#include "radapter-broker/debugging/logginginterceptor.h"
#include "radapter-broker/broker.h"

using namespace Redis;

PubSubFactory::PubSubFactory(const QList<Settings::RedisKeyEventSubscriber> &subscribers,
                                       QObject *parent)
    : FactoryBase(parent),
      m_subscribersList(subscribers)
{
}

int PubSubFactory::initWorkers()
{
    if (m_subscribersList.isEmpty()) {
        return 0;
    }

    for (auto &subscriberInfo : m_subscribersList) {
        auto thread = new QThread{};
        auto settings = Radapter::WorkerSettings();
        QSet<Radapter::InterceptorBase*> loggers;
        if (subscriberInfo.log_jsons) {
            loggers.insert(new Radapter::LoggingInterceptor(*subscriberInfo.log_jsons));
        }
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        auto consumer = new KeyEventsConsumer(subscriberInfo.source_server.server_host,
                                              subscriberInfo.source_server.server_port,
                                              subscriberInfo.keyEvents,
                                              subscriberInfo.worker,
                                              thread);
        connect(thread, &QThread::started, consumer, &KeyEventsConsumer::run);
        connect(thread, &QThread::finished, consumer, &KeyEventsConsumer::deleteLater);
        m_workersPool.insert(consumer);
        Radapter::Broker::instance()->registerProxy(consumer->createProxy(loggers));
    }
    return 0;
}

Radapter::WorkersList PubSubFactory::getWorkers() const
{
    return m_workersPool;
}


void PubSubFactory::run()
{
    for (auto &worker : m_workersPool) {
        worker->thread()->start();
    }
}
