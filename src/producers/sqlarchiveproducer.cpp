#include "sqlarchiveproducer.h"

using namespace MySql;

ArchiveProducer::ArchiveProducer(const Settings::SqlStorageInfo &archiveInfo, QThread *thread)
    : Radapter::Worker(archiveInfo.worker, thread),
      m_dbClient(new Connector(Settings::SqlClientInfo::get(archiveInfo.client_name), this)),
      m_archiveInfo(archiveInfo)
{
}

void ArchiveProducer::onMsg(const Radapter::WorkerMsg &msg)
{
    Q_UNUSED(msg)
    throw std::runtime_error("Not implemented!");
}

