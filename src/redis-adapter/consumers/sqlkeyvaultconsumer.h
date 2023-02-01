#ifndef SQLKEYVAULTCONSUMER_H
#define SQLKEYVAULTCONSUMER_H

#include <QObject>
#include "redis-adapter/connectors/mysqlconnector.h"
#include "redis-adapter/settings/settings.h"
#include "radapter-broker/workerbase.h"

namespace Sql {
class RADAPTER_SHARED_SRC KeyVaultConsumer : public Radapter::WorkerBase
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

    MySqlConnector* m_dbClient;
    Settings::SqlStorageInfo m_info;
    QVariantList m_tableFields;
    JsonDict m_cache;
    QTimer* m_pollTimer;
};
}
#endif // SQLKEYVAULTCONSUMER_H
