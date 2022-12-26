#include "sqlqueryfieldsformatter.h"

using namespace MySql;

SqlQueryFieldsFormatter::SqlQueryFieldsFormatter(const JsonDict &queryFields, QObject *parent)
    : QObject(parent),
      m_jsonQueryFields(queryFields)
{
}

SqlQueryFieldsFormatter::SqlQueryFieldsFormatter(const QVariantList &fieldNamesList, QObject *parent)
    : QObject(parent),
      m_jsonQueryFields{}
{
    for (auto &fieldName : fieldNamesList) {
        m_jsonQueryFields.insert(fieldName.toString(), QVariant{});
    }
}

MySql::QueryRecord SqlQueryFieldsFormatter::toSqlRecord() const
{
    auto sqlRecord = QueryRecord{};
    for (auto jsonField = m_jsonQueryFields.begin();
         jsonField != m_jsonQueryFields.end();
         jsonField++)
    {
        auto sqlField = QueryField{ jsonField.key(), jsonField.value() };
        sqlRecord.append(sqlField);
    }
    return sqlRecord;
}

QueryRecordList SqlQueryFieldsFormatter::toSqlRecordList() const
{
    auto recordsList = QueryRecordList{ toSqlRecord() };
    return recordsList;
}
