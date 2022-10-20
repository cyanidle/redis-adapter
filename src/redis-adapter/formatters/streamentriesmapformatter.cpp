#include "streamentriesmapformatter.h"
#include "redisstreamentryformatter.h"

StreamEntriesMapFormatter::StreamEntriesMapFormatter(const Formatters::Dict &streamEntriesJson, QObject *parent)
    : QObject(parent),
      m_streamEntriesMap(streamEntriesJson)
{
}

bool StreamEntriesMapFormatter::isValid(const Formatters::Dict &streamEntriesJson)
{
    if (streamEntriesJson.isEmpty()) {
        return false;
    }
    auto firstItem = Formatters::Dict{ QVariantMap{ { streamEntriesJson.firstKey(), streamEntriesJson.first() } } };
    auto eventTime = RedisStreamEntryFormatter(firstItem).eventTime();
    return eventTime.isValid();
}

Formatters::List StreamEntriesMapFormatter::toEntryList()
{
    auto entryList = Formatters::List{};
    for (auto streamItem = m_streamEntriesMap.begin();
         streamItem != m_streamEntriesMap.end();
         streamItem++)
    {
        auto streamEntry = Formatters::Dict{ QVariantMap{ { streamItem.key(), streamItem.value() } } };
        entryList.append(streamEntry);
    }
    return entryList;
}

Formatters::Dict StreamEntriesMapFormatter::joinToLatest()
{
    auto mergedJson = Formatters::Dict{};
    auto entryList = toEntryList();
    for (auto &jsonEntry : entryList) {
        auto streamEntryData = RedisStreamEntryFormatter(Formatters::Dict{ jsonEntry }).eventDataDict();
        mergedJson.merge(streamEntryData);
    }
    return mergedJson;
}

