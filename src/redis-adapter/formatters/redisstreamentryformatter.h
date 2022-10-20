#ifndef REDISSTREAMENTRYFORMATTER_H
#define REDISSTREAMENTRYFORMATTER_H

#include <QObject>
#include <QDateTime>
#include "json-formatters/formatters/dict.h"
#include "lib/hiredis/hiredis.h"

class RADAPTER_SHARED_SRC RedisStreamEntryFormatter : public QObject
{
    Q_OBJECT
public:
    explicit RedisStreamEntryFormatter(const Formatters::Dict &redisStreamJsonEntry, QObject *parent = nullptr);
    explicit RedisStreamEntryFormatter(redisReply* streamReply, QObject *parent = nullptr);

    QString entryId() const;
    QDateTime eventTime() const;
    QStringList entryKeys() const;
    Formatters::Dict eventDataDict() const;
    Formatters::Dict toJson() const;
signals:

private:
    Formatters::Dict m_streamEntry;
};

#endif // REDISSTREAMENTRYFORMATTER_H
