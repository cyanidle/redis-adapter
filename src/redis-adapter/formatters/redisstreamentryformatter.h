#ifndef REDISSTREAMENTRYFORMATTER_H
#define REDISSTREAMENTRYFORMATTER_H

#include <QObject>
#include <QDateTime>
#include "jsondict/jsondict.hpp"
#include "lib/hiredis/hiredis.h"

class RADAPTER_SHARED_SRC RedisStreamEntryFormatter
{
public:
    explicit RedisStreamEntryFormatter(const JsonDict &streamReply);
    explicit RedisStreamEntryFormatter(JsonDict &&streamReply);
    explicit RedisStreamEntryFormatter(redisReply* streamReply);
    QString entryId() const;
    QDateTime eventTime() const;
    QStringList entryKeys() const;
    JsonDict eventDataDict() const;
    JsonDict toJson() const;
private:
    JsonDict m_streamEntry;
};

#endif // REDISSTREAMENTRYFORMATTER_H
