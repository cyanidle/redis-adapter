#include "sqlresultformatter.h"

using namespace MySql;

SqlResultFormatter::SqlResultFormatter(const MySql::QueryRecordList &resultRecords, QObject *parent)
    : QObject(parent),
      m_sqlRecords(resultRecords)
{
}

JsonDict SqlResultFormatter::toJsonRecord(const MySql::QueryRecord &sqlRecord) const
{
    auto jsonRecord = JsonDict{};
    for (auto &sqlField : sqlRecord) {
        jsonRecord.insert(sqlField.name, sqlField.value);
    }
    return jsonRecord;
}

QVariantList SqlResultFormatter::toJsonList() const
{
    auto jsonRecordsList = QVariantList{};
    for (auto &sqlRecord : m_sqlRecords) {
        auto jsonRecord = toJsonRecord(sqlRecord);
        jsonRecordsList.append(jsonRecord.data());
    }
    return jsonRecordsList;
}

