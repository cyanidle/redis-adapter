#ifndef MYSQLCONNECTOR_H
#define MYSQLCONNECTOR_H

#include <QObject>
#include "JsonFormatters"
#include "lib/mysql/mysqlclient.h"
#include "radapter-broker/workermsg.h"

class RADAPTER_SHARED_SRC MySqlConnector : public QObject
{
    Q_OBJECT
public:
    explicit MySqlConnector(const Settings::SqlClientInfo &clientInfo, QObject *parent = nullptr);

    Formatters::List doRead(const QString &tableName, const Formatters::List &fieldsList, const QString &sqlConditions = QString{});
    bool doWrite(const QString &tableName, const Formatters::Dict &jsonRecord, const Radapter::WorkerMsg &msg);
    bool doWriteList(const QString &tableName, const Formatters::List &jsonRecords, const Radapter::WorkerMsg &msg);
    const QString &name() const {return m_clientInfo.name;}

signals:
    void connectedChanged(bool isConnected);
    void readDone(const QString &tableName, const Formatters::List &jsonRecordsList);
    void writeDone(bool isOk, const Radapter::WorkerMsg &msg);

public slots:
    void run();

private:
    Settings::SqlClientInfo m_clientInfo;
    MySqlClient* m_client;
};

#endif // MYSQLCONNECTOR_H
