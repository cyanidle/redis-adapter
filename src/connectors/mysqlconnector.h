#ifndef MYSQLCONNECTOR_H
#define MYSQLCONNECTOR_H

#include <QObject>
#include "jsondict/jsondict.h"
#include "mysql/mysqlclient.h"

namespace MySql {
class RADAPTER_API Connector : public QObject
{
    Q_OBJECT
public:
    explicit Connector(const Settings::SqlClientInfo &clientInfo, QObject *parent);

    QVariantList doRead(const QString &tableName, const QStringList &fieldsList, const QString &sqlConditions = QString{});
    bool doWrite(const QString &tableName, const JsonDict &jsonRecord, void* data);
    bool doWriteList(const QString &tableName, const QVariantList &jsonRecords, void* data);
    const QString &name() const {return m_clientInfo.name;}

signals:
    void connectedChanged(bool isConnected);
    void readDone(const QString &tableName, const QVariantList &jsonRecordsList);
    void writeDone(bool isOk, void* data);

public slots:
    void onRun();

private:
    Settings::SqlClientInfo m_clientInfo;
    MySql::Client* m_client;
};
}

#endif // MYSQLCONNECTOR_H
