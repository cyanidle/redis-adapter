#include "streamentriesmapformatter.h"
#include "redisstreamentryformatter.h"

StreamEntriesMapFormatter::StreamEntriesMapFormatter(const JsonDict &streamEntriesJson, QObject *parent)
    : QObject(parent),
      m_streamEntriesMap(streamEntriesJson)
{
}

bool StreamEntriesMapFormatter::isValid(const JsonDict &streamEntriesJson)
{
    if (streamEntriesJson.isEmpty()) {
        return false;
    }
    auto firstItem = JsonDict{ QVariantMap{ { streamEntriesJson.firstKey(), streamEntriesJson.first() } } };
    auto eventTime = RedisStreamEntryFormatter(firstItem).eventTime();
    return eventTime.isValid();
}

QVariantList StreamEntriesMapFormatter::toEntryList()
{
    auto entryList = QVariantList{};
    for (auto streamItem = m_streamEntriesMap.begin();
         streamItem != m_streamEntriesMap.end();
         streamItem++)
    {
        auto streamEntry = JsonDict{ { streamItem.key().join(":"), streamItem.value() } } ;
        entryList.append(streamEntry.toVariant());
    }
    return entryList;
}

 JsonDict StreamEntriesMapFormatter::joinToLatest()
{
    auto mergedJson = JsonDict{};
    auto entryList = toEntryList();
    for (auto &jsonEntry : entryList) {
        auto streamEntryData = RedisStreamEntryFormatter( JsonDict{ jsonEntry }).eventDataDict();
        mergedJson.merge(streamEntryData);
    }
    return mergedJson;
}

