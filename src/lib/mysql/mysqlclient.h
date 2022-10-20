#ifndef MYSQLCLIENT_H
#define MYSQLCLIENT_H

#include <QObject>
#include <QtSql>
#include "redis-adapter/settings/settings.h"

namespace MySql {
    struct RADAPTER_SHARED_SRC QueryField {
        QString name;
        QVariant value;
        bool isFunction;

        QueryField(const QString &fieldName, const QVariant &fieldValue, const bool isCommand = false) {
            name = fieldName;
            value = fieldValue;
            isFunction = isCommand;
        }
        QueryField() : QueryField(QString{}, QVariant{}) {}

        bool isValid() {
            return !name.isEmpty() && value.isValid();
        }

        QMetaType::Type type() const {
            return static_cast<QMetaType::Type>(value.type());
        }

        QString toString() const {
            switch (type()) {
            case QMetaType::QDateTime:
                if (value.isNull()) {
                    return QString("NULL");
                } else {
                    return value.toDateTime().toString("''yyyy-MM-dd hh:mm:ss''");
                }
            case QMetaType::QString:
                if (isFunction) {
                    return value.toString();
                }
                return QString("'%1'").arg(value.toString());
            default:
                return value.toString();
            }
        }

        bool operator==(const QueryField &other) {
            return this->name == other.name
                    && this->value == other.value
                    && this->isFunction == other.isFunction;
        }
    };
    typedef QList<QueryField> QueryRecord;
    typedef QList<QueryRecord> QueryRecordList;
    typedef QList<QVariantList> RecordValuesMap;
}

class RADAPTER_SHARED_SRC MySqlClient : public QObject
{
    Q_OBJECT
public:

    explicit MySqlClient(const Settings::SqlClientInfo &clientInfo,
                         QObject *parent = nullptr);
    ~MySqlClient() override;

    bool open();
    bool isOpened() const;
    bool doSelectQuery(const QString &tableName, MySql::QueryRecordList &recordList, const QString &conditions = QString());
    bool doUpdateQuery(const QString &tableName, const MySql::QueryRecord &updateRecord, const QString &conditions);
    bool doInsertQuery(const QString &tableName, const MySql::QueryRecord &insertRecord, bool updateOnDuplicate = false);
    bool doDeleteQuery(const QString &tableName, const MySql::QueryRecord &deleteRecord);

    static MySql::QueryField getQueryField(const MySql::QueryRecord &record, const QString &name);

    static bool selectAtOnce(const Settings::SqlClientInfo &clientInfo,
                           const QString &tableName,
                           MySql::QueryRecordList &recordList,
                           const QString &conditions = QString());

signals:
    void connected(bool state);

public slots:
    void reopen();

private slots:
    void setConnected(bool state);

private:
    static QMap<QString, QString> usernameMap();
    static QString createSelectString(const QString &dbName,
                                      const QString &tableName,
                                      const MySql::QueryRecord &header,
                                      const QString &conditions);
    static MySql::RecordValuesMap readResultRecords(QSqlQuery &finishedQuery);

    void doReopen();
    MySql::RecordValuesMap execSelectQuery(const QString &query, bool &isOk);
    bool execQuery(const QString &query);
    static MySql::QueryRecordList makeRecordList(const MySql::QueryRecord &fieldNames, const MySql::RecordValuesMap valuesMap);

    QSqlDatabase m_db;
    QMap<QString, QString> m_usernameMap;
    QTimer* m_reconnectTimer;
    bool m_isConnected;
};

Q_DECLARE_METATYPE(MySql::QueryField);

#endif // MYSQLCLIENT_H
