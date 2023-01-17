#ifndef MYSQLCLIENT_H
#define MYSQLCLIENT_H

#include <QObject>
#include <QtSql>
#include "redis-adapter/settings/settings.h"

namespace MySql {
    struct RADAPTER_SHARED_SRC QueryField : Serializer::SerializableGadget {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, name, DEFAULT)
        SERIAL_FIELD(QVariant, value, DEFAULT)
        SERIAL_FIELD(bool, isFunction, false)
        QueryField(const QString &fieldName, const QVariant &fieldValue, const bool isCommand = false);
        QueryField();
        bool isValid();
        QMetaType::Type type() const;
        QString toString() const;
        bool operator==(const QueryField &other);
    };


    typedef QList<QueryField> QueryRecord;
    typedef QList<QueryRecord> QueryRecordList;
    typedef QList<QVariantList> RecordValuesMap;


    class RADAPTER_SHARED_SRC Client : public QObject
    {
        Q_OBJECT
    public:

    explicit Client(const Settings::SqlClientInfo &clientInfo,
                         QObject *parent = nullptr);
    ~Client() override;

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






inline QueryField::QueryField(const QString &fieldName, const QVariant &fieldValue, const bool isCommand) :
    name(fieldName),
    value(fieldValue),
    isFunction(isCommand)
{}

inline QueryField::QueryField() : QueryField(QString{}, QVariant{}) {}

inline bool QueryField::isValid() {
    return !name.isEmpty() && value.isValid();
}

inline QMetaType::Type QueryField::type() const {
    return static_cast<QMetaType::Type>(value.type());
}

inline QString QueryField::toString() const {
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

inline bool QueryField::operator==(const QueryField &other) {
    return this->name == other.name
           && this->value == other.value
           && this->isFunction == other.isFunction;
}
}
Q_DECLARE_METATYPE(MySql::QueryField);
Q_DECLARE_TYPEINFO(MySql::QueryField, Q_MOVABLE_TYPE);

#endif // MYSQLCLIENT_H
