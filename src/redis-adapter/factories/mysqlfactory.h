#ifndef MYSQLFACTORY_H
#define MYSQLFACTORY_H

#include <QObject>
#include <QThread>
#include "radapter-broker/factorybase.h"
#include "redis-adapter/settings/settings.h"
#include "redis-adapter/connectors/mysqlconnector.h"

class RADAPTER_SHARED_SRC MySqlFactory : public Radapter::FactoryBase
{
    Q_OBJECT
public:
    explicit MySqlFactory(const QList<Settings::SqlClientInfo> &sqlClients, QObject *parent = nullptr);

    QList<MySqlConnector*> getWorkers() const;
    MySqlConnector* getWorker(const QString &name);
    int initWorkers() override;
    int initSettings() override {return 0;}
    void run() override;
signals:

private:
    QList<MySqlConnector*> m_workersPool;
    QList<Settings::SqlClientInfo> m_clientsInfoList;
};

#endif // MYSQLFACTORY_H
