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

    JsonDict readJsonEntries(const QStringList &keys);

public slots:
    void run();

private:



    MySqlConnector* m_dbClient;
    Settings::SqlKeyVaultInfo m_info;
    QVariantList m_tableFields;
};

#endif // SQLKEYVAULTCONSUMER_H
