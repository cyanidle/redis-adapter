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
#include "broker/workers/workersettings.h"
#include "settings-parsing/serializablesettings.h"
#include <QJsonDocument>

Q_DECLARE_METATYPE(QTimeZone)
Q_DECLARE_METATYPE(QDataStream::ByteOrder)

namespace Settings {
struct ByteOrderValidator {
    typedef QMap<QString, QDataStream::ByteOrder> Map;
    static bool validate(QVariant &src) {
        static Map map{
            {"little",  QDataStream::LittleEndian},
            {"big",  QDataStream::BigEndian},
            {"littleendian",  QDataStream::LittleEndian},
            {"bigendian", QDataStream::BigEndian}
        };
        auto asStr = src.toString().toLower();
        src.setValue(map.value(asStr));
        return map.contains(asStr);
    }
};
using NonRequiredByteOrder = Serializable::Validated<NonRequired<QDataStream::ByteOrder>>::With<ByteOrderValidator>;

struct RADAPTER_API Pipelines : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(NonRequiredSequence<QString>, pipelines)
};

struct RADAPTER_API ServerInfo : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<QString>, host)
    FIELD(Required<quint16>, port)
};

struct RADAPTER_API TcpDevice : public ServerInfo {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(NonRequired<QString>, name)


    operator bool() const {return port;}
    typedef QMap<QString, TcpDevice> Map;
    POST_UPDATE {
        if (!name->isEmpty()) {
            table().insert(name, *this);
        }
    }
    static bool has(const QString &name) {
        return table().contains(name);
    }
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

struct RADAPTER_API SerialDevice : SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<QString>, port_name)
    FIELD(NonRequired<QString>, name)
    FIELD(NonRequired<int>, parity, {QSerialPort::NoParity})
    FIELD(NonRequired<int>, baud, {QSerialPort::Baud115200})
    FIELD(NonRequired<int>, data_bits, {QSerialPort::Data8})
    FIELD(NonRequired<int>, stop_bits, {QSerialPort::OneStop})
    FIELD(NonRequiredByteOrder, byte_order, {QDataStream::BigEndian})


    operator bool() const {return !port_name->isEmpty();}
    typedef QMap<QString, SerialDevice> Map;
    POST_UPDATE {
        if (!name->isEmpty()) {
            table().insert(name, *this);
        }
    }
    static bool has(const QString &name) {
        return table().contains(name);
    }
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


struct RADAPTER_API SqlClientInfo : ServerInfo
{
    typedef QMap<QString, SqlClientInfo> Map;
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<QString>, name)
    FIELD(Required<QString>, database)
    FIELD(Required<QString>, username)
    POST_UPDATE {
        table().insert(name, *this);
    }
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

struct RADAPTER_API SqlStorageInfo : SerializableSettings
{
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<Radapter::WorkerSettings>, worker)
    FIELD(Required<QString>, name)
    FIELD(Required<QString>, client_name)
    FIELD(Required<QString>, target_table)
    FIELD(Required<QString>, table_name)
};

struct RADAPTER_API Filters {
    typedef QMap<QString /*filterName*/, double> Table;
    typedef QMap<QString /*filterName*/, Table> TableMap;
    static TableMap &table() {
        static TableMap map{};
        return map;
    }
};

struct ValidateTimeZone {
    static bool validate(QVariant& target) {
        auto time_zone = QTimeZone(target.toString().toStdString().c_str());
        target.setValue(time_zone);
        return time_zone.isValid();
    }
};

struct RADAPTER_API LocalizationInfo : SerializableSettings {
    using RequiredTimeZone = Serializable::Validated<Required<QTimeZone>>::With<ValidateTimeZone>;

    Q_GADGET
    IS_SERIALIZABLE
    FIELD(RequiredTimeZone, time_zone)
};

struct RADAPTER_API WebsocketServerInfo : SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<quint16>, port, {1234})
    FIELD(Required<Radapter::WorkerSettings>, worker)
};

struct RADAPTER_API WebsocketClientInfo : WebsocketServerInfo {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<QString>, host)
};

} // namespace Settings

#endif // SETTINGS_H
