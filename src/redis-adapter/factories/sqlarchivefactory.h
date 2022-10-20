#ifndef SQLARCHIVEFACTORY_H
#define SQLARCHIVEFACTORY_H

#include <QObject>
#include <QThread>
#include "redis-adapter/settings/settings.h"
#include "mysqlfactory.h"
#include "redis-adapter/producers/sqlarchiveproducer.h"
#include "redis-adapter/factories/mysqlfactory.h"

namespace Sql {
class RADAPTER_SHARED_SRC ArchiveFactory;
}

class Sql::ArchiveFactory : public Radapter::FactoryBase
{
    Q_OBJECT
public:
    explicit ArchiveFactory(const QList<Settings::SqlStorageInfo> &archivesInfo,
                            MySqlFactory *dbFactory,
                            QObject *parent = nullptr);

    int initSettings() override {return 0;}
    int initWorkers() override;
    void run() override;
    Radapter::WorkersList getWorkers() const;
    void startWorkers();

signals:
private:
    QList<Settings::SqlStorageInfo> m_archivesInfoList;
    Radapter::WorkersList m_workerPool;
    MySqlFactory* m_dbFactory;
};

#endif // SQLARCHIVEFACTORY_H
