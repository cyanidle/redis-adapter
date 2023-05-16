#ifndef SQLKEYVAULTCONSUMER_H
#define SQLKEYVAULTCONSUMER_H

#include "broker/workers/worker.h"
#include "connectors/mysqlconnector.h"
#include "settings/settings.h"
class QTimer;
namespace MySql {
class RADAPTER_API KeyVaultConsumer : public Radapter::Worker
{
    Q_OBJECT
public:
    explicit KeyVaultConsumer(const Settings::SqlStorageInfo &info, QThread *thread);
public slots:
    void run();

private slots:
    void updateCache();

private:
    JsonDict readSqlEntries();

    MySql::Connector* m_dbClient;
    Settings::SqlStorageInfo m_info;
    QStringList m_tableFields;
    JsonDict m_cache;
    QTimer* m_pollTimer;
};
}
#endif // SQLKEYVAULTCONSUMER_H
