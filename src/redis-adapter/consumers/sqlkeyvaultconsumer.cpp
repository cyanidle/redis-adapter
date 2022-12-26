#include "sqlkeyvaultconsumer.h"
#include "redis-adapter/formatters/sqlqueryformatter.h"
#include "redis-adapter/formatters/keyvaultresultformatter.h"
#include "redis-adapter/include/sqlkeyvaultfields.h"

using namespace Sql;

KeyVaultConsumer::KeyVaultConsumer(MySqlConnector *client,
                                         const Settings::SqlKeyVaultInfo &info,
                                         QObject *parent) :
    QObject(parent),
    m_dbClient(client),
    m_info(info)
{
    m_tableFields = QVariantList{
            SQL_KEYVAULT_FIELD_REDIS_KEY,
            SQL_KEYVAULT_FIELD_SOURCE_TYPE,
            SQL_KEYVAULT_FIELD_SOURCE,
            SQL_KEYVAULT_FIELD_PARAM
    };
}

void KeyVaultConsumer::run()
{
}

JsonDict KeyVaultConsumer::readJsonEntries(const QStringList &keys)
{   if (keys.isEmpty()) {
        return JsonDict();
    }
    auto keysFilter = SqlQueryFormatter().toRegExpFilter(SQL_KEYVAULT_FIELD_REDIS_KEY, keys);
    auto sqlEntries = m_dbClient->doRead(m_info.table_name, m_tableFields, keysFilter);
    if (sqlEntries.isEmpty()) {
        return JsonDict{};
    }
    auto keyedEntriesDict = KeyVaultResultFormatter(sqlEntries).toJsonEntries();
//    if (!keyedEntriesDict.isEmpty()) {
//        emit msgToBroker(prepareMsg(keyedEntriesDict));
//    }
    return keyedEntriesDict;
}
