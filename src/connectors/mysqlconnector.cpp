#include "mysqlconnector.h"
#include "formatting/sql/sqlresults.h"
#include "jsondict/jsondict.h"
using namespace MySql;
Connector::Connector(const Settings::SqlClientInfo &clientInfo, QObject *parent)
    : QObject(parent),
      m_clientInfo(clientInfo),
      m_client(nullptr)
{
}

void Connector::onRun()
{
    if (m_client && m_client->isOpened()) {
        return;
    }

    m_client = new MySql::Client(m_clientInfo, this);
    connect(m_client, &MySql::Client::connected, this, &Connector::connectedChanged);
    m_client->open();
}

QVariantList Connector::doRead(const QString &tableName, const QStringList &fieldsList, const QString &sqlConditions)
{
    if (fieldsList.isEmpty()) {
        return QVariantList{};
    }

    auto jsonRecords = JsonDict{};
    auto sqlRecordsList = QueryRecordList{{}};
    for (const auto &field : fieldsList) {
        sqlRecordsList[0].append(field);
    }
    bool isOk = m_client->doSelectQuery(tableName, sqlRecordsList, sqlConditions);
    if (!isOk) {
        return QVariantList{};
    }
    QVariantList jsonResult;
    for (auto &record : sqlRecordsList) {
        QVariantMap subresult;
        for (auto &field : record) {
            subresult.insert(field.name, field.value);
        }
        jsonResult.append(subresult);
    }
    if (!jsonResult.isEmpty()) {
        emit readDone(tableName, jsonResult);
    }
    return jsonResult;
}

bool Connector::doWrite(const QString &tableName, const JsonDict &jsonRecord, void* data)
{
    auto jsonQuery = QVariantList{};
    jsonQuery.append(jsonRecord.toVariant());
    bool isOk = doWriteList(tableName, jsonQuery, data);
    return isOk;
}

bool Connector::doWriteList(const QString &tableName, const QVariantList &jsonRecords, void* data)
{
    if (jsonRecords.isEmpty()) {
        return false;
    }
    bool isOk = true;
    for (auto &recordToWrite : jsonRecords) {
        QueryRecord record;
        auto currentMap = recordToWrite.toMap();
        for (auto field = currentMap.begin(); field != currentMap.end(); ++field) {
            record.append({field.key(), field.value()});
        }
        isOk &= m_client->doInsertQuery(tableName, record);
    }
    emit writeDone(isOk, data);
    return isOk;
}
