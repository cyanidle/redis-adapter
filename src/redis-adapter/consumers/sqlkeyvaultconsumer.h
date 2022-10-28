#ifndef SQLKEYVAULTCONSUMER_H
#define SQLKEYVAULTCONSUMER_H

#include <QObject>
#include "redis-adapter/connectors/mysqlconnector.h"
#include "radapter-broker/workerbase.h"
#include "redis-adapter/settings/settings.h"

namespace Sql {
class RADAPTER_SHARED_SRC KeyVaultConsumer;
}

class Sql::KeyVaultConsumer : public QObject
{
    Q_OBJECT
public:
    explicit KeyVaultConsumer(MySqlConnector *client,
                              const Settings::SqlKeyVaultInfo &info,
                              QObject *parent = nullptr);

    Formatters::Dict readJsonEntries(const QStringList &keys);

public slots:
//    Commands
    void run();
//    void onCommand(const Radapter::WorkerMsg &msg) override;

private:



    MySqlConnector* m_dbClient;
    Settings::SqlKeyVaultInfo m_info;
    Formatters::List m_tableFields;
};

#endif // SQLKEYVAULTCONSUMER_H
