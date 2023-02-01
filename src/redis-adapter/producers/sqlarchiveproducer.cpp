#include "sqlarchiveproducer.h"
#include "redis-adapter/formatters/redisstreamentryformatter.h"
#include "redis-adapter/formatters/streamentriesmapformatter.h"
#include "redis-adapter/formatters/archivequeryformatter.h"

using namespace Sql;

ArchiveProducer::ArchiveProducer(const Settings::SqlStorageInfo &archiveInfo, QThread *thread)
    : Radapter::WorkerBase(archiveInfo.worker, thread),
      m_dbClient(new MySqlConnector(Settings::SqlClientInfo::table().value(archiveInfo.client_name), this)),
      m_archiveInfo(archiveInfo)
{
}

void ArchiveProducer::onMsg(const Radapter::WorkerMsg &msg)
{

}

