#include "mysqlfactory.h"
#include "radapter-broker/broker.h"

MySqlFactory::MySqlFactory(const QList<Settings::SqlClientInfo> &sqlClients, QObject *parent)
    : FactoryBase(parent),
      m_workersPool{},
      m_clientsInfoList(sqlClients)
{
}

//! Can return null!
MySqlConnector* MySqlFactory::getWorker(const QString &name)
{
    for (auto &client : m_workersPool) {
        if (client->name() == name) {
            return client;
        }
    }
    return nullptr;
}

int MySqlFactory::initWorkers()
{
    for (auto &clientInfo : m_clientsInfoList) {
        auto thread = new QThread(this);
        QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        auto connector = new MySqlConnector(clientInfo);
        connector->moveToThread(thread);
        QObject::connect(thread, &QThread::started, connector, &MySqlConnector::run);
        QObject::connect(thread, &QThread::finished, connector, &MySqlConnector::deleteLater);
        m_workersPool.append(connector);
    }
    return 0;
}

QList<MySqlConnector*> MySqlFactory::getWorkers() const
{
    return m_workersPool;
}


void MySqlFactory::run()
{
    for (auto &worker : m_workersPool) {
        worker->thread()->start();
    }
}
