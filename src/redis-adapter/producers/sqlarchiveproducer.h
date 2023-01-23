#ifndef SQLARCHIVEPRODUCER_H
#define SQLARCHIVEPRODUCER_H

#include <QObject>
#include "redis-adapter/settings/settings.h"
#include "radapter-broker/workerbase.h"
#include "redis-adapter/connectors/mysqlconnector.h"
#include "redis-adapter/consumers/sqlkeyvaultconsumer.h"

namespace Sql {
class RADAPTER_SHARED_SRC ArchiveProducer;
}

class Sql::ArchiveProducer : public Radapter::WorkerBase
{
    Q_OBJECT
public:
    explicit ArchiveProducer(MySqlConnector *client,
                             const Settings::SqlStorageInfo &archiveInfo,
                             const Radapter::WorkerSettings &settings, QThread *thread);
public slots:
    void onRun() override;
    void onMsg(const Radapter::WorkerMsg &msg) override;
    //Replies
    void saveFinished(bool isOk, const Radapter::WorkerMsg &msg);
private:
    //Generic Data In
    void saveRedisStreamEntries(const JsonDict &redisStreamJson, const Radapter::WorkerMsg &msg);

    MySqlConnector* m_dbClient;
    KeyVaultConsumer* m_keyVaultClient;
    Settings::SqlStorageInfo m_archiveInfo;
};

#endif // SQLARCHIVEPRODUCER_H
