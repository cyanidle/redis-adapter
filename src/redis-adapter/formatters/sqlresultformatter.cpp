#include "sqlresultformatter.h"

using namespace MySql;

SqlResultFormatter::SqlResultFormatter(const MySql::QueryRecordList &resultRecords, QObject *parent)
    : QObject(parent),
      m_sqlRecords(resultRecords)
{
}

Formatters::Dict SqlResultFormatter::toJsonRecord(const MySql::QueryRecord &sqlRecord) const
{
    auto jsonRecord = Formatters::Dict{};
    for (auto &sqlField : sqlRecord) {
        jsonRecord.insert(sqlField.name, sqlField.value);
    }
    return jsonRecord;
}

Formatters::List SqlResultFormatter::toJsonList() const
{
    auto jsonRecordsList = Formatters::List{};
    for (auto &sqlRecord : m_sqlRecords) {
        auto jsonRecord = toJsonRecord(sqlRecord);
        jsonRecordsList.append(jsonRecord);
    }
    return jsonRecordsList;
}

