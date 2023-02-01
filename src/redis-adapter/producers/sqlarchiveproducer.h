#ifndef SQLARCHIVEPRODUCER_H
#define SQLARCHIVEPRODUCER_H

#include <QObject>
#include "redis-adapter/settings/settings.h"
#include "radapter-broker/workerbase.h"
#include "redis-adapter/consumers/sqlkeyvaultconsumer.h"
#include "redis-adapter/connectors/mysqlconnector.h"

namespace Sql {
class RADAPTER_SHARED_SRC ArchiveProducer;
}

class Sql::ArchiveProducer : public Radapter::WorkerBase
{
    Q_OBJECT
public:
    explicit ArchiveProducer(const Settings::SqlStorageInfo &archiveInfo, QThread *thread);
public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
private:
    MySqlConnector* m_dbClient;
    Settings::SqlStorageInfo m_archiveInfo;
};

#endif // SQLARCHIVEPRODUCER_H
