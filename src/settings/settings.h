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
    static bool validate(QVariant &src, const QVariantList &args, QVariant &state) {
        Q_UNUSED(args)
        Q_UNUSED(state)
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
using NonRequiredByteOrder = Serializable::Validated<HasDefault<QDataStream::ByteOrder>>::With<ByteOrderValidator>;

struct RADAPTER_API Pipelines : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(SequenceHasDefault<QString>, pipelines)
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
    FIELD(HasDefault<QString>, name)
    typedef QMap<QString, TcpDevice> Map;
    bool isValid () const {
        return port > 0;
    }
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
    FIELD(HasDefault<QString>, name)
    FIELD(HasDefault<int>, parity, QSerialPort::NoParity)
    FIELD(HasDefault<int>, baud, QSerialPort::Baud115200)
    FIELD(HasDefault<int>, data_bits, QSerialPort::Data8)
    FIELD(HasDefault<int>, stop_bits, QSerialPort::OneStop)
    FIELD(NonRequiredByteOrder, byte_order, QDataStream::BigEndian)

    typedef QMap<QString, SerialDevice> Map;
    bool isValid () const {
        return !port_name->isEmpty();
    }
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
    FIELD(Required<Worker>, worker)
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
    static bool validate(QVariant& target, const QVariantList &args, QVariant &state) {
        Q_UNUSED(args)
        Q_UNUSED(state)
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

struct RADAPTER_API WebsocketServer : SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<Worker>, worker)

    FIELD(HasDefault<quint16>, port, 1234)
    FIELD(HasDefault<quint16>, heartbeat_ms, 10000)
    FIELD(HasDefault<quint16>, keepalive_time, 20000)
    FIELD(HasDefault<QString>, bind_to, "0.0.0.0")
    FIELD(HasDefault<QString>, name, "redis-adapter")
    FIELD(HasDefault<bool>, secure, false)

    POST_UPDATE {
        if (heartbeat_ms >= keepalive_time) {
            throw std::runtime_error("Cannot have 'heartbeat_ms' bigger than 'keepalive_time'");
        }
    }
};

struct RADAPTER_API WebsocketClient : WebsocketServer {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<QString>, host)
};

} // namespace Settings

#endif // SETTINGS_H
