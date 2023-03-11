#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSerialPort>
#include <QVariant>
#include <QDataStream>
#include <QVector>
#include <QTime>
#include <QModbusDataUnit>
#include <QTimeZone>
#include "broker/worker/workersettings.h"
#include "settings-parsing/serializablesettings.h"
#include <QJsonDocument>

Q_DECLARE_METATYPE(QTimeZone)

namespace Settings {

struct RADAPTER_SHARED_SRC Pipelines : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    SERIAL_CONTAINER(QList, QString, pipelines, DEFAULT)
};

struct RADAPTER_SHARED_SRC ServerInfo : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    SERIAL_FIELD(QString, host)
    SERIAL_FIELD(quint16, port)
};

struct RADAPTER_SHARED_SRC TcpDevice : public ServerInfo {
    typedef QMap<QString, TcpDevice> Map;
    Q_GADGET
    IS_SERIALIZABLE
    SERIAL_FIELD(QString, name, DEFAULT)
    SERIAL_POST_INIT([this](){if (!name.isEmpty()) table().insert(name, *this);})
    static bool has(const QString &name) {return table().contains(name);}
    static const TcpDevice &get(const QString &name) {
        if (!table().contains(name)) throw std::invalid_argument("Missing TCP Device: " + name.toStdString());
        return table()[name];
    }
protected:
    static Map &table() {
        static Map map{};
        return map;
    }
};

struct RADAPTER_SHARED_SRC SerialDevice : SerializableSettings {
    typedef QMap<QString, SerialDevice> Map;
    Q_GADGET
    IS_SERIALIZABLE
    SERIAL_FIELD(QString, port_name);

    SERIAL_FIELD(QString, name, DEFAULT);
    SERIAL_FIELD(int, parity, QSerialPort::NoParity)
    SERIAL_FIELD(int, baud, QSerialPort::Baud115200)
    SERIAL_FIELD(int, data_bits, QSerialPort::Data8)
    SERIAL_FIELD(int, stop_bits, QSerialPort::OneStop)
    SERIAL_POST_INIT([this](){if (!name.isEmpty()) table().insert(name, *this);})
    static bool has(const QString &name) {return table().contains(name);}
    static const SerialDevice &get(const QString &name) {
        if (!table().contains(name)) throw std::invalid_argument("Missing SERIAL Device: " + name.toStdString());
        return table()[name];
    }
protected:
    static Map &table() {
        static Map map{};
        return map;
    }
};


struct RADAPTER_SHARED_SRC SqlClientInfo : ServerInfo {
    typedef QMap<QString, SqlClientInfo> Map;
    Q_GADGET
    IS_SERIALIZABLE
    SERIAL_FIELD(QString, name)
    SERIAL_FIELD(QString, database)
    SERIAL_FIELD(QString, username)
    SERIAL_POST_INIT([this](){table().insert(name, *this);})
    static const SqlClientInfo &get(const QString& name) {
        if (!table().contains(name)) throw std::invalid_argument("Missing Sql Client with name: " + name.toStdString());
        return table()[name];
    }
protected:
    static Map& table() {
        static Map map{};
        return map;
    }
};

struct RADAPTER_SHARED_SRC SqlStorageInfo : SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    SERIAL_FIELD(Radapter::WorkerSettings, worker)
    SERIAL_FIELD(QString, name)
    SERIAL_FIELD(QString, client_name)
    SERIAL_FIELD(QString, target_table)
    SERIAL_FIELD(QString, table_name);
};

struct RADAPTER_SHARED_SRC Filters {
    typedef QMap<QString /*filterName*/, double> Table;
    typedef QMap<QString /*filterName*/, Table> TableMap;
    static TableMap &table() {
        static TableMap map{};
        return map;
    }
};

struct RADAPTER_SHARED_SRC LocalizationInfo : SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    SERIAL_CUSTOM(QTimeZone, time_zone, initTimeZone, readTimeZone, QTimeZone::systemTimeZone());
    bool initTimeZone(const QVariant &src) {
        time_zone = QTimeZone(src.toString().toStdString().c_str());
        return time_zone.isValid();
    }
    QVariant readTimeZone() const {
        return time_zone.displayName(QTimeZone::TimeType::GenericTime, QTimeZone::NameType::DefaultName);
    }
    bool isValid() const {
        return time_zone.isValid();
    }
};

struct RADAPTER_SHARED_SRC WebsocketServerInfo : SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    SERIAL_FIELD(quint16, port, 1234)
    SERIAL_FIELD(Radapter::WorkerSettings, worker)
    bool isValid() const {
        return worker.isValid();
    }
};

struct RADAPTER_SHARED_SRC WebsocketClientInfo : WebsocketServerInfo {
    Q_GADGET
    IS_SERIALIZABLE
    SERIAL_FIELD(QString, host)
};
}

#endif // SETTINGS_H
