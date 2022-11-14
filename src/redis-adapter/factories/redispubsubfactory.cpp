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
        settings.isDebug = subscriberInfo.debug;
        settings.thread = thread;
        settings.name = subscriberInfo.name;
        settings.consumers = subscriberInfo.consumers;
        settings.producers = subscriberInfo.producers;
        QList<Radapter::InterceptorBase*> loggers;
        if (subscriberInfo.log_jsons.use) {
            loggers.append(new Radapter::LoggingInterceptor(subscriberInfo.log_jsons.asSettings()));
        }
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        auto consumer = new KeyEventsConsumer(subscriberInfo.source_server.server_host,
                                              subscriberInfo.source_server.server_port,
                                              subscriberInfo.keyEvents,
                                              settings);
        connect(thread, &QThread::started, consumer, &KeyEventsConsumer::run);
        connect(thread, &QThread::finished, consumer, &KeyEventsConsumer::deleteLater);
        m_workersPool.append(consumer);
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
