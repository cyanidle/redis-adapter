#include "websocketclientfactory.h"
#include "radapter-broker/broker.h"

using namespace Websocket;

ClientFactory::ClientFactory(const QList<Settings::WebsockerClientInfo> &info, QObject *parent) :
    Radapter::FactoryBase(parent),
    m_info(info)
{   
}

int ClientFactory::initWorkers()
{
    for (auto &info : m_info) {
        auto settings = Radapter::WorkerSettings{
            info.name,
            new QThread(this),
            info.consumers,
            info.producers,
            info.debug
        };
        connect(settings.thread, &QThread::finished, settings.thread, &QThread::deleteLater);
        auto client = new Websocket::Client(info.server_host, info.server_port, settings);
        connect(client->thread(), &QThread::started, client, &Websocket::Client::start);
        connect(client->thread(), &QThread::finished, client, &Websocket::Client::deleteLater);
        m_workersPool.append(client);
        Radapter::Broker::instance()->registerProxy(client->createProxy());
    }
    return 0;
}

Radapter::WorkersList ClientFactory::getWorkers() const
{
    return m_workersPool;
}

void ClientFactory::run()
{
    for (auto &worker : m_workersPool) {
        worker->thread()->start();
    }
}
