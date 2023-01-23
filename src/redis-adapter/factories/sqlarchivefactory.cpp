#include "sqlarchivefactory.h"
#include <radapter-broker/broker.h>

using namespace Sql;

ArchiveFactory::ArchiveFactory(const QList<Settings::SqlStorageInfo> &archivesInfo,
                               MySqlFactory *dbFactory,
                               QObject *parent)
    : FactoryBase(parent),
      m_archivesInfoList(archivesInfo),
      m_workerPool{},
      m_dbFactory(dbFactory)
{
}

int ArchiveFactory::initWorkers()
{
    for (auto &archiveInfo : m_archivesInfoList) {
        if (!archiveInfo.isValid()) {
            return -1;
        }
        auto dbWorker = m_dbFactory->getWorker(archiveInfo.client_name);
        auto archiveProducer = new ArchiveProducer(dbWorker, archiveInfo, archiveInfo.worker, new QThread(this));
        connect(archiveProducer->thread(), &QThread::started, archiveProducer, &ArchiveProducer::run);
        connect(archiveProducer->thread(), &QThread::finished, archiveProducer, &ArchiveProducer::deleteLater);
        m_workerPool.insert(archiveProducer);
        Radapter::Broker::instance()->registerProxy(archiveProducer->createProxy());
    }
    return 0;
}

Radapter::WorkersList ArchiveFactory::getWorkers() const
{
    return m_workerPool;
}

void ArchiveFactory::run()
{
    for (auto &worker : qAsConst(m_workerPool)) {
        worker->thread()->start();
    }
}
