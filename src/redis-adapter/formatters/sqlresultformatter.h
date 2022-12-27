#ifndef SQLRESULTFORMATTER_H
#define SQLRESULTFORMATTER_H

#include "jsondict/jsondict.hpp"
#include "lib/mysql/mysqlclient.h"

class RADAPTER_SHARED_SRC SqlResultFormatter : public QObject
{
public:
    explicit SqlResultFormatter(const MySql::QueryRecordList &resultRecords, QObject *parent = nullptr);

    QVariantList toJsonList() const;
signals:

private:
    JsonDict toJsonRecord(const MySql::QueryRecord &sqlRecord) const;

    MySql::QueryRecordList m_sqlRecords;
};

#endif // SQLRESULTFORMATTER_H
