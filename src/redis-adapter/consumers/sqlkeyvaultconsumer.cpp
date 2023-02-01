#include "sqlkeyvaultconsumer.h"
#include "redis-adapter/formatters/keyvaultresultformatter.h"
#include "redis-adapter/include/sqlkeyvaultfields.h"
#include "redis-adapter/radapterlogging.h"

using namespace Sql;

#define KEYVAULT_POLL_RATE_MS   60000

KeyVaultConsumer::KeyVaultConsumer(const Settings::SqlStorageInfo &info, QThread *thread) :
    WorkerBase(info.worker, thread),
    m_dbClient(new MySqlConnector(Settings::SqlClientInfo::table().value(info.client_name), this)),
    m_info(info),
    m_cache{},
    m_pollTimer(nullptr)
{
    m_tableFields = QVariantList{ QVariantList{
            SQL_KEYVAULT_FIELD_REDIS_KEY,
            SQL_KEYVAULT_FIELD_SOURCE_TYPE,
            SQL_KEYVAULT_FIELD_SOURCE,
            SQL_KEYVAULT_FIELD_PARAM,
            SQL_KEYVAULT_ATTR_EVENT_TIME
    } };
}

void KeyVaultConsumer::run()
{
    m_pollTimer = new QTimer(this);
    m_pollTimer->setSingleShot(false);
    m_pollTimer->setInterval(KEYVAULT_POLL_RATE_MS);
    m_pollTimer->callOnTimeout(this, &KeyVaultConsumer::updateCache);
    m_pollTimer->start();
    updateCache();
}

JsonDict KeyVaultConsumer::readSqlEntries()
{
    auto sqlEntries = m_dbClient->doRead(m_info.table_name, m_tableFields);
    if (sqlEntries.isEmpty()) {
        return JsonDict{};
    }
    auto keyedEntriesDict = KeyVaultResultFormatter(sqlEntries).toJsonEntries();
    return keyedEntriesDict;
}

void KeyVaultConsumer::updateCache()
{
    auto keyVaultEntries = readSqlEntries();
    if (keyVaultEntries.isEmpty()) {
        return;
    }
    m_cache = keyVaultEntries;
}

