#ifndef MYSQLRESULTFORMATTER_H
#define MYSQLRESULTFORMATTER_H

#include "JsonFormatters"
#include "lib/mysql/mysqlclient.h"

class RADAPTER_SHARED_SRC SqlQueryFieldsFormatter : public QObject
{
public:
    explicit SqlQueryFieldsFormatter(const Formatters::Dict &queryFields, QObject *parent = nullptr);
    explicit SqlQueryFieldsFormatter(const Formatters::List &fieldNamesList, QObject *parent = nullptr);

    MySql::QueryRecord toSqlRecord() const;
    MySql::QueryRecordList toSqlRecordList() const;
signals:

private:
    Formatters::Dict m_jsonQueryFields;
};

#endif // MYSQLRESULTFORMATTER_H
