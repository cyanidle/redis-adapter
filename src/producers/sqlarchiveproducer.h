#ifndef SQLARCHIVEPRODUCER_H
#define SQLARCHIVEPRODUCER_H

#include <QObject>
#include "settings/settings.h"
#include "broker/workers/worker.h"
#include "consumers/sqlkeyvaultconsumer.h"
#include "connectors/mysqlconnector.h"

namespace MySql {
class RADAPTER_API ArchiveProducer;
}

class MySql::ArchiveProducer : public Radapter::Worker
{
    Q_OBJECT
public:
    explicit ArchiveProducer(const Settings::SqlStorageInfo &archiveInfo, QThread *thread);
public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
private:
    Connector* m_dbClient;
    Settings::SqlStorageInfo m_archiveInfo;
};

#endif // SQLARCHIVEPRODUCER_H
