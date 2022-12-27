#ifndef REDISSTREAMENTRYFORMATTER_H
#define REDISSTREAMENTRYFORMATTER_H

#include <QObject>
#include <QDateTime>
#include "jsondict/jsondict.hpp"
#include "lib/hiredis/hiredis.h"

class RADAPTER_SHARED_SRC RedisStreamEntryFormatter : public QObject
{
    Q_OBJECT
public:
    explicit RedisStreamEntryFormatter(const JsonDict &redisStreamJsonEntry, QObject *parent = nullptr);
    explicit RedisStreamEntryFormatter(redisReply* streamReply, QObject *parent = nullptr);

    QString entryId() const;
    QDateTime eventTime() const;
   QStringList entryKeys() const;
    JsonDict eventDataDict() const;
    JsonDict toJson() const;
signals:

private:
    JsonDict m_streamEntry;
};

#endif // REDISSTREAMENTRYFORMATTER_H
