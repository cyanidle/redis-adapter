#include "redisstreamentryformatter.h"
#include "timeformatter.h"

RedisStreamEntryFormatter::RedisStreamEntryFormatter(const Formatters::Dict &redisStreamJsonEntry, QObject *parent)
    : QObject(parent),
      m_streamEntry(redisStreamJsonEntry)
{
}

RedisStreamEntryFormatter::RedisStreamEntryFormatter(redisReply *streamReply, QObject *parent)
    : QObject(parent),
      m_streamEntry{}
{
    auto entryId = QString(streamReply->element[0]->str);
    if (entryId.isEmpty()) {
        return;
    }
    auto itemData = streamReply->element[1];
    if (itemData->elements == 0) {
        return;
    }

    auto fields = Formatters::Dict{};
    for (quint16 i = 0; i < itemData->elements; i += 2) {
        auto fieldKey = QString(itemData->element[i]->str);
        auto fieldValue = QString(itemData->element[i + 1]->str);
        if (!fieldValue.isEmpty()) {
            fields[fieldKey] = fieldValue;
        }
    }
    if (!fields.isEmpty()) {
        auto jsonEntry = Formatters::Dict{};
        jsonEntry.insert(entryId, fields);
        m_streamEntry = jsonEntry;
    }
}

QString RedisStreamEntryFormatter::entryId() const
{
    auto entryId = m_streamEntry.firstKey();
    return entryId;
}

QDateTime RedisStreamEntryFormatter::eventTime() const
{
    auto timestamp = entryId().split("-").first().toLongLong();
    auto eventTime = TimeFormatter(timestamp).dateTime();
    return eventTime;
}

QStringList RedisStreamEntryFormatter::entryKeys() const
{
    auto keysList = eventDataDict().keys();
    return QStringList(keysList);
}

Formatters::Dict RedisStreamEntryFormatter::eventDataDict() const
{
    auto keyValueDict = Formatters::Dict(m_streamEntry.first());
    return keyValueDict;
}

Formatters::Dict RedisStreamEntryFormatter::toJson() const
{
    return m_streamEntry;
}
