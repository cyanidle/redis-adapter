#ifndef MYSQLCONNECTOR_H
#define MYSQLCONNECTOR_H

#include <QObject>
#include "json-formatters/formatters/list.h"
#include "lib/mysql/mysqlclient.h"

class RADAPTER_SHARED_SRC MySqlConnector : public QObject
{
    Q_OBJECT
public:
    explicit MySqlConnector(const Settings::SqlClientInfo &clientInfo, QObject *parent = nullptr);

    Formatters::List doRead(const QString &tableName, const Formatters::List &fieldsList, const QString &sqlConditions = QString{});
    bool doWrite(const QString &tableName, const Formatters::Dict &jsonRecord, quint64 msgId);
    bool doWriteList(const QString &tableName, const Formatters::List &jsonRecords, quint64 msgId);
    const QString &name() const {return m_clientInfo.name;}

signals:
    void connectedChanged(bool isConnected);
    void readDone(const QString &tableName, const Formatters::List &jsonRecordsList);
    void writeDone(bool isOk, quint64 msgId);

public slots:
    void run();

private:
    Settings::SqlClientInfo m_clientInfo;
    MySqlClient* m_client;
};

#endif // MYSQLCONNECTOR_H
