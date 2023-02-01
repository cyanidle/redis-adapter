#include "mysqlclient.h"
#include "redis-adapter/radapterlogging.h"

using namespace MySql;

#define CONNECTION_TIMED_OUT_ID "2006"
#define CONNECTION_LOST_ID      "2013"
#define REOPEN_DELAY_MS         3000
#define TEMPORARY_DB_CONNECTION "temp"

Client::Client(const Settings::SqlClientInfo &clientInfo, QObject *parent)
    : QObject(parent),
      m_isConnected(false)
{
    m_db = QSqlDatabase::addDatabase("QMYSQL", clientInfo.name);
    m_db.setHostName(clientInfo.host);
    m_db.setPort(clientInfo.port);
    m_db.setDatabaseName(clientInfo.database);
    m_db.setUserName(clientInfo.username);
    m_db.setPassword(usernameMap().value(clientInfo.username));

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(REOPEN_DELAY_MS);
    m_reconnectTimer->callOnTimeout(this, &MySql::Client::doReopen);

    connect(this, &MySql::Client::connected,
            this, &MySql::Client::reopen);
}

MySql::Client::~Client()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    m_db.removeDatabase(m_db.connectionName());
}

QMap<QString, QString> MySql::Client::usernameMap()
{
    static auto usernameMap = QMap<QString, QString>{
        { "lotos", "mysql" },
        { "root", QString{} }
    };
    return usernameMap;
}

bool MySql::Client::open()
{
    auto opened = m_db.open();
    sqlDebug() << "MYSQL:" << QString("%1@%2").arg(m_db.databaseName(), m_db.hostName())
              << "is opened:" << opened;
    setConnected(opened);

    if (!m_db.isOpen()) {
        reopen();
    }
    return opened;
}

void MySql::Client::reopen()
{
    if (!m_isConnected && !m_reconnectTimer->isActive()) {
        m_reconnectTimer->start();
    }
}

void MySql::Client::doReopen()
{
    m_db.close();
    open();
}

bool MySql::Client::isOpened() const
{
    return m_db.isOpen();
}

void MySql::Client::setConnected(bool state)
{
    if (m_isConnected != state) {
        m_isConnected = state;

        if (m_isConnected) {
            m_reconnectTimer->stop();
        }

        emit connected(m_isConnected);
    }
}

bool MySql::Client::doSelectQuery(const QString &tableName, QueryRecordList &recordList, const QString &conditions)
{
    auto header = recordList.takeFirst();
    auto queryString = createSelectString(m_db.databaseName(), tableName, header, conditions);

    bool isOk = false;
    auto recordValuesMap = execSelectQuery(queryString, isOk);
    if (!isOk) {
        return false;
    }

    if (!recordValuesMap.isEmpty()) {
        recordList = makeRecordList(header, recordValuesMap);
    }
    return true;
}

bool MySql::Client::doUpdateQuery(const QString &tableName, const QueryRecord &updateRecord, const QString &conditions)
{
    auto queryString = QString("UPDATE `%1`.`%2` SET ").arg(m_db.databaseName(), tableName);
    auto separator = QString(", ");
    for (auto field : updateRecord) {
        auto fieldSetString = QString("`%1`=%2").arg(field.name, field.toString());
        queryString.append(fieldSetString + separator);
    }
    queryString.chop(separator.size());
    queryString.append(" " + conditions);

    bool isOk = execQuery(queryString);
    return isOk;
}

bool MySql::Client::doInsertQuery(const QString &tableName, const QueryRecord &insertRecord, bool updateOnDuplicate)
{
    auto queryString = QString("INSERT INTO `%1`.`%2` (").arg(m_db.databaseName(), tableName);
    auto separator = QString(", ");
    for (auto &field : insertRecord) {
        auto fieldName = QString("`%1`").arg(field.name);
        queryString.append(fieldName + separator);
    }
    queryString.chop(separator.size());
    queryString.append(") VALUES(");
    for (auto &field : insertRecord) {
        auto fieldValue = field.toString();
        queryString.append(fieldValue + separator);
    }
    queryString.chop(separator.size());
    queryString.append(")");

    if (updateOnDuplicate) {
        queryString.append(" ON DUPLICATE KEY UPDATE ");
        for (auto &field : insertRecord) {
            auto fieldString = QString("`%1` = %2").arg(field.name, field.toString());
            queryString.append(fieldString + separator);
        }
        queryString.chop(separator.size());
    }

    bool isOk = execQuery(queryString);
    return isOk;
}

bool MySql::Client::doDeleteQuery(const QString &tableName, const QueryRecord &deleteRecord)
{
    auto queryString = QString("DELETE FROM `%1`.`%2` WHERE ")
            .arg(m_db.databaseName(), tableName);
    auto separator = QString("AND ");
    for (auto &field : deleteRecord) {
        auto fieldString = QString("`%1` = %2 ").arg(field.name, field.toString());
        queryString.append(fieldString + separator);
    }
    queryString.chop(separator.size());

    bool isOk = execQuery(queryString);
    return isOk;
}

QString MySql::Client::createSelectString(const QString &dbName, const QString &tableName, const MySql::QueryRecord &header, const QString &conditions)
{
    auto queryString = QString("SELECT ");
    auto separator = QString(", ");
    for (auto &field : header) {
        queryString.append(field.name + separator);
    }
    queryString.chop(separator.size());
    queryString.append(QString(" FROM `%1`.`%2` ").arg(dbName, tableName));
    if (!conditions.isEmpty()) {
        queryString.append(conditions);
    }
    return queryString;
}

RecordValuesMap MySql::Client::readResultRecords(QSqlQuery &finishedQuery)
{
    auto recordsMap = RecordValuesMap{};
    while (finishedQuery.next()) {
        auto record = QVariantList();
        auto fieldsCount = finishedQuery.record().count();
        for (quint8 fieldIndex = 0u; fieldIndex < fieldsCount; fieldIndex++) {
            auto fieldValue = finishedQuery.value(fieldIndex);
            if (fieldValue.isValid()) {
                record.append(fieldValue);
            }
        }
        recordsMap.append(record);
    }
    return recordsMap;
}

RecordValuesMap MySql::Client::execSelectQuery(const QString &query, bool &isOk)
{
    sqlDebug() << query;
    auto sqlQuery = QSqlQuery(query, m_db);
    isOk = sqlQuery.isActive();
    sqlDebug() << QThread::currentThreadId() << Q_FUNC_INFO << "query is active:" << isOk;
    if (!isOk) {
        sqlDebug() << sqlQuery.lastQuery() << ": query failed " << sqlQuery.lastError();
        sqlDebug() << Q_FUNC_INFO << sqlQuery.lastError().text();
        auto connectionError = sqlQuery.lastError().type() == QSqlError::ConnectionError;
        auto connectionTimedOut = sqlQuery.lastError().nativeErrorCode() == CONNECTION_TIMED_OUT_ID;
        auto connectionLost = sqlQuery.lastError().nativeErrorCode() == CONNECTION_LOST_ID;
        if (connectionError || connectionTimedOut || connectionLost) {
            setConnected(false);
        }
        return RecordValuesMap{};
    }

    sqlDebug() << QThread::currentThreadId() << "[MySQL]::SELECT" << "reading query records...";
    auto recordsMap = readResultRecords(sqlQuery);
    sqlDebug() << QThread::currentThreadId() << "[MySQL]::SELECT" << "reading complete.";

    isOk = true;
    return recordsMap;
}

bool MySql::Client::execQuery(const QString &query)
{
    sqlDebug() << query;
    auto sqlQuery = QSqlQuery(m_db);
    bool isOk = sqlQuery.exec(query);
    if (!isOk) {
        sqlDebug() << sqlQuery.lastQuery() << ": query failed " << sqlQuery.lastError();
        sqlDebug() << Q_FUNC_INFO << sqlQuery.lastError().text();
        auto connectionError = sqlQuery.lastError().type() == QSqlError::ConnectionError;
        auto connectionTimedOut = sqlQuery.lastError().nativeErrorCode() == CONNECTION_TIMED_OUT_ID;
        auto connectionLost = sqlQuery.lastError().nativeErrorCode() == CONNECTION_LOST_ID;
        if (connectionError || connectionTimedOut || connectionLost) {
            setConnected(false);
        }
    }
    return isOk;
}

QueryRecordList MySql::Client::makeRecordList(const QueryRecord &fieldNames, const RecordValuesMap valuesMap)
{
    auto recordList = QueryRecordList{};
    auto skipIdColumn = fieldNames.count() != valuesMap.first().count();
    for (auto &recordValues : valuesMap) {
        auto record = QueryRecord();
        quint8 valueIndex = skipIdColumn ? 1u : 0u;
        for (quint8 fieldNameIndex = 0u; fieldNameIndex < fieldNames.count(); fieldNameIndex++, valueIndex++) {
            auto fieldName = fieldNames.at(fieldNameIndex).name;
            auto field = QueryField{ fieldName, recordValues.at(valueIndex) };
            record.append(field);
        }
        recordList.append(record);
    }
    return recordList;
}

QueryField MySql::Client::getQueryField(const QueryRecord &record, const QString &name)
{
    for (auto field : record) {
        if (field.name == name) {
            return field;
        }
    }
    return QueryField{};
}

bool MySql::Client::selectAtOnce(const Settings::SqlClientInfo &clientInfo, const QString &tableName, MySql::QueryRecordList &recordList, const QString &conditions)
{
    bool isOk = false;
    {
        auto dbClient = QSqlDatabase::addDatabase("QMYSQL", TEMPORARY_DB_CONNECTION);
        dbClient.setHostName(clientInfo.host);
        dbClient.setPort(clientInfo.port);
        dbClient.setDatabaseName(clientInfo.database);
        isOk = dbClient.open(clientInfo.username, usernameMap().value(clientInfo.username));
        if (isOk) {
            auto header = recordList.takeFirst();
            auto queryString = createSelectString(dbClient.databaseName(), tableName, header, conditions);
            auto sqlQuery = QSqlQuery(queryString, dbClient);
            isOk = sqlQuery.isActive();
            if (isOk) {
                auto recordValuesMap = readResultRecords(sqlQuery);
                if (!recordValuesMap.isEmpty()) {
                    recordList = makeRecordList(header, recordValuesMap);
                }
            } else {
                sqlDebug() << sqlQuery.lastQuery() << ": query failed " << sqlQuery.lastError();
                sqlDebug() << Q_FUNC_INFO << sqlQuery.lastError().text();
            }
        } else {
            sqlDebug() << Q_FUNC_INFO << dbClient.lastError().text();
        }
    }
    QSqlDatabase::removeDatabase(TEMPORARY_DB_CONNECTION);
    return isOk;
}
