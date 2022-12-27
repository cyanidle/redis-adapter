#ifndef MYSQLRESULTFORMATTER_H
#define MYSQLRESULTFORMATTER_H

#include "jsondict/jsondict.hpp"
#include "lib/mysql/mysqlclient.h"

class RADAPTER_SHARED_SRC SqlQueryFieldsFormatter : public QObject
{
public:
    explicit SqlQueryFieldsFormatter(const JsonDict &queryFields, QObject *parent = nullptr);
    explicit SqlQueryFieldsFormatter(const QVariantList &fieldNamesList, QObject *parent = nullptr);

    MySql::QueryRecord toSqlRecord() const;
    MySql::QueryRecordList toSqlRecordList() const;
signals:

private:
    JsonDict m_jsonQueryFields;
};

#endif // MYSQLRESULTFORMATTER_H
