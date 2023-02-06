#include "sqlkeyvaultconsumer.h"
#include "radapterlogging.h"

using namespace MySql;

#define KEYVAULT_POLL_RATE_MS   60000

KeyVaultConsumer::KeyVaultConsumer(const Settings::SqlStorageInfo &info, QThread *thread) :
    Worker(info.worker, thread),
    m_dbClient(new Connector(Settings::SqlClientInfo::get(info.client_name), this)),
    m_info(info),
    m_cache{},
    m_pollTimer(nullptr)
{

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
    //! \todo Finish
    return {};
}

void KeyVaultConsumer::updateCache()
{
    auto keyVaultEntries = readSqlEntries();
    if (keyVaultEntries.isEmpty()) {
        return;
    }
    m_cache = keyVaultEntries;
}

