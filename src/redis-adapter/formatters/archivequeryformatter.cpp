#include "archivequeryformatter.h"
#include "redisstreamentryformatter.h"
#include "keyvaultresultformatter.h"
#include "timeformatter.h"
#include "redis-adapter/include/sqlarchivefields.h"

ArchiveQueryFormatter::ArchiveQueryFormatter(const Formatters::Dict &redisStreamEntry, QObject *parent)
    : QObject(parent),
      m_streamEntry(redisStreamEntry)
{
}

Formatters::List ArchiveQueryFormatter::toWriteRecordsList(const Formatters::Dict &keyVaultEntries) const
{
    RedisStreamEntryFormatter formatter(m_streamEntry);
    auto itemsToWrite = formatter.eventDataDict();
    auto eventTime = formatter.eventTime();
    auto serverTime = TimeFormatter{}.now();
    auto writeRecordsList = Formatters::List{};
    for (auto writeItem = itemsToWrite.begin();
         writeItem != itemsToWrite.end();
         writeItem++)
    {
        auto associatedKeyBindings = Formatters::Dict(keyVaultEntries.value(writeItem.key()));
        bool isValidEntry = KeyVaultResultFormatter{}.isValid(associatedKeyBindings);
        if (!isValidEntry) {
            continue;
        }
        auto writeRecord = Formatters::Dict{};
        writeRecord.insert(SQL_ARCHIVE_FIELD_SOURCE_TYPE,
                                  associatedKeyBindings.value(SQL_ARCHIVE_FIELD_SOURCE_TYPE));
        writeRecord.insert(SQL_ARCHIVE_FIELD_SOURCE,
                                  associatedKeyBindings.value(SQL_ARCHIVE_FIELD_SOURCE));
        writeRecord.insert(SQL_ARCHIVE_FIELD_PARAM,
                                  associatedKeyBindings.value(SQL_ARCHIVE_FIELD_PARAM));
        writeRecord.insert(SQL_ARCHIVE_FIELD_EVENT_TIME, eventTime);
        writeRecord.insert(SQL_ARCHIVE_FIELD_SERVER_TIME, serverTime);
        writeRecord.insert(SQL_ARCHIVE_FIELD_VALUE, writeItem.value());
        writeRecordsList.append(writeRecord);
    }
    return writeRecordsList;
}
