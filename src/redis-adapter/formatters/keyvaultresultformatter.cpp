#include "keyvaultresultformatter.h"
#include "redis-adapter/include/sqlkeyvaultfields.h"

KeyVaultResultFormatter::KeyVaultResultFormatter(const QVariantList &sqlRecordsList, QObject *parent)
    : QObject(parent),
      m_sqlRecordsList(sqlRecordsList)
{
}

 JsonDict KeyVaultResultFormatter::toJsonEntries() const
{
    auto keyedEntriesDict = JsonDict{};
    for (auto &keyVaultEntry : m_sqlRecordsList) {
        auto jsonItem = JsonDict(keyVaultEntry);
        auto redisKey = jsonItem.take(SQL_KEYVAULT_FIELD_REDIS_KEY).toString();
        keyedEntriesDict.insert(redisKey, jsonItem);
    }
    return keyedEntriesDict;
}

bool KeyVaultResultFormatter::isValid(const JsonDict &keyVaultEntry) const
{
    auto keyBindings = keyVaultEntry;
    auto sourceType = keyBindings.take(SQL_KEYVAULT_FIELD_SOURCE_TYPE);
    auto source = keyBindings.take(SQL_KEYVAULT_FIELD_SOURCE);
    auto param = keyBindings.take(SQL_KEYVAULT_FIELD_PARAM);
    if (!(sourceType.toUInt() > 0u) || !(source.toUInt() > 0u) || !(param.toUInt() > 0u)) {
        return false;
    }
    return keyBindings.isEmpty();
}
