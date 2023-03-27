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
using NonRequiredByteOrder = Serializable::Validated<NonRequiredField<QDataStream::ByteOrder>>::With<ByteOrderValidator>;

struct RADAPTER_API Pipelines : public SerializableSettings {
    Q_GADGET
    FIELDS(pipelines)
    NonRequiredSequence<QString> pipelines;
};

struct RADAPTER_API ServerInfo : public SerializableSettings {
    Q_GADGET
    FIELDS(host, port)
    RequiredField<QString> host;
    RequiredField<quint16> port;
};

struct RADAPTER_API TcpDevice : public ServerInfo {
    typedef QMap<QString, TcpDevice> Map;
    Q_GADGET
    FIELDS(name)
    operator bool() const {return port;}
    NonRequiredField<QString> name;
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
    typedef QMap<QString, SerialDevice> Map;
    Q_GADGET
    FIELDS(port_name, name, parity, baud, data_bits, stop_bits, byte_order)
    operator bool() const {return !port_name->isEmpty();}
    RequiredField<QString> port_name;
    NonRequiredField<QString> name;
    NonRequiredField<int> parity {QSerialPort::NoParity};
    NonRequiredField<int> baud {QSerialPort::Baud115200};
    NonRequiredField<int> data_bits {QSerialPort::Data8};
    NonRequiredField<int> stop_bits {QSerialPort::OneStop};
    NonRequiredByteOrder byte_order {QDataStream::BigEndian};
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


struct RADAPTER_API SqlClientInfo : ServerInfo {
    typedef QMap<QString, SqlClientInfo> Map;
    Q_GADGET
    FIELDS(name, database, username)
    RequiredField<QString> name;
    RequiredField<QString> database;
    RequiredField<QString> username;
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

struct RADAPTER_API SqlStorageInfo : SerializableSettings {
    Q_GADGET
    FIELDS(worker, name, client_name, target_table, table_name)
    RequiredField<Radapter::WorkerSettings> worker;
    RequiredField<QString> name;
    RequiredField<QString> client_name;
    RequiredField<QString> target_table;
    RequiredField<QString> table_name;
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
    Q_GADGET
    FIELDS(time_zone)
    using RequiredTimeZone = Serializable::Validated<RequiredField<QTimeZone>>::With<ValidateTimeZone>;
    RequiredTimeZone time_zone;
};

struct RADAPTER_API WebsocketServerInfo : SerializableSettings {
    Q_GADGET
    FIELDS(port, worker)
    RequiredField<quint16> port{1234};
    RequiredField<Radapter::WorkerSettings> worker;
};

struct RADAPTER_API WebsocketClientInfo : WebsocketServerInfo {
    Q_GADGET
    FIELDS(host)
    RequiredField<QString> host;
};

} // namespace Settings

#endif // SETTINGS_H
