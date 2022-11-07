#ifndef SQLRESULTFORMATTER_H
#define SQLRESULTFORMATTER_H

#include "JsonFormatters"
#include "lib/mysql/mysqlclient.h"

class RADAPTER_SHARED_SRC SqlResultFormatter : public QObject
{
public:
    explicit SqlResultFormatter(const MySql::QueryRecordList &resultRecords, QObject *parent = nullptr);

    Formatters::List toJsonList() const;
signals:

private:
    Formatters::Dict toJsonRecord(const MySql::QueryRecord &sqlRecord) const;

    MySql::QueryRecordList m_sqlRecords;
};

#endif // SQLRESULTFORMATTER_H
