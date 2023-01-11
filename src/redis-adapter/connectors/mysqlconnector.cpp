#include "mysqlconnector.h"
#include "redis-adapter/formatters/sqlqueryfieldsformatter.h"
#include "redis-adapter/formatters/sqlresultformatter.h"

MySqlConnector::MySqlConnector(const Settings::SqlClientInfo &clientInfo, QObject *parent)
    : QObject(parent),
      m_clientInfo(clientInfo),
      m_client(nullptr)
{
}

void MySqlConnector::run()
{
    if (m_client && m_client->isOpened()) {
        return;
    }

    m_client = new MySqlClient(m_clientInfo, this);
    connect(m_client, &MySqlClient::connected, this, &MySqlConnector::connectedChanged);
    m_client->open();
}

QVariantList MySqlConnector::doRead(const QString &tableName, const QVariantList &fieldsList, const QString &sqlConditions)
{
    if (fieldsList.isEmpty()) {
        return QVariantList{};
    }

    auto jsonRecords = JsonDict{};
    auto sqlRecordsList = SqlQueryFieldsFormatter(fieldsList).toSqlRecordList();
    bool isOk = m_client->doSelectQuery(tableName, sqlRecordsList, sqlConditions);
    if (!isOk) {
        return QVariantList{};
    }
    auto jsonResult = SqlResultFormatter(sqlRecordsList).toJsonList();
    if (!jsonResult.isEmpty()) {
        emit readDone(tableName, jsonResult);
    }
    return jsonResult;
}

bool MySqlConnector::doWrite(const QString &tableName, const JsonDict &jsonRecord, const Radapter::WorkerMsg &msg)
{
    auto jsonQuery = QVariantList{};
    jsonQuery.append(jsonRecord.toVariant());
    bool isOk = doWriteList(tableName, jsonQuery, msg);
    return isOk;
}

bool MySqlConnector::doWriteList(const QString &tableName, const QVariantList &jsonRecords, const Radapter::WorkerMsg &msg)
{
    if (jsonRecords.isEmpty()) {
        return false;
    }

    bool isOk = true;
    for (auto &recordToWrite : jsonRecords) {
        auto writeRecord = SqlQueryFieldsFormatter( JsonDict(recordToWrite)).toSqlRecord();
        isOk &= m_client->doInsertQuery(tableName, writeRecord);
    }
    emit writeDone(isOk, msg);
    return isOk;
}
