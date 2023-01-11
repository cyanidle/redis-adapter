#include "sqlarchiveproducer.h"
#include "redis-adapter/formatters/redisstreamentryformatter.h"
#include "redis-adapter/formatters/streamentriesmapformatter.h"
#include "redis-adapter/formatters/archivequeryformatter.h"

using namespace Sql;

ArchiveProducer::ArchiveProducer(MySqlConnector *client,
                                 const Settings::SqlStorageInfo &archiveInfo,
                                 const Radapter::WorkerSettings &settings,
                                 QThread *thread)
    : Radapter::WorkerBase(settings, thread),
      m_dbClient(client),
      m_keyVaultClient(nullptr),
      m_archiveInfo(archiveInfo)
{
}

void ArchiveProducer::run()
{
    m_keyVaultClient = new KeyVaultConsumer(m_dbClient, m_archiveInfo.key_vault, this);
    connect(m_dbClient, &MySqlConnector::writeDone,
            this, &ArchiveProducer::saveFinished);
    m_keyVaultClient->run();
}

void ArchiveProducer::onMsg(const Radapter::WorkerMsg &msg)
{
    saveRedisStreamEntries(msg , msg);
}

void ArchiveProducer::saveFinished(bool isOk, const Radapter::WorkerMsg &msg)
{
    auto reply = prepareReply(msg, isOk ? Radapter::WorkerMsg::ReplyOk : Radapter::WorkerMsg::ReplyFail);
    emit sendMsg(reply);
}

void ArchiveProducer::saveRedisStreamEntries(const JsonDict &redisStreamJson, const Radapter::WorkerMsg &msg)
{
    auto streamEntriesList = StreamEntriesMapFormatter(redisStreamJson).toEntryList();
    for (auto &streamEntry : streamEntriesList) {
        auto entryKeys = RedisStreamEntryFormatter( JsonDict(streamEntry)).entryKeys();
        auto keyVaultEntries = m_keyVaultClient->readJsonEntries(entryKeys);
        auto recordsToWrite = ArchiveQueryFormatter( JsonDict(streamEntry)).toWriteRecordsList(keyVaultEntries);
        m_dbClient->doWriteList(m_archiveInfo.target_table, recordsToWrite, msg);
    }
}
