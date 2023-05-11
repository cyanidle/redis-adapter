#ifndef SETTINGS_H
#define SETTINGS_H

#include "broker/workers/settings/workersettings.h"
#include <QSerialPort>
#include <QTimeZone>

Q_DECLARE_METATYPE(QTimeZone)
Q_DECLARE_METATYPE(QDataStream::ByteOrder)

namespace Validator {
struct ByteOrder {
    static const QString &name();
    static bool validate(QVariant &src);
};

struct TimeZone {
    static const QString &name();
    static bool validate(QVariant& target);
};
}

namespace Settings {
using NonRequiredByteOrder = ::Serializable::Validated<HasDefault<QDataStream::ByteOrder>>::With<Validator::ByteOrder>;
using RequiredTimeZone = ::Serializable::Validated<Settings::Required<QTimeZone>>::With<Validator::TimeZone>;

struct RADAPTER_API Pipelines : public Serializable
{
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(SequenceHasDefault<QString>, pipelines)
};

struct RADAPTER_API ServerInfo : public Serializable
{
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<QString>, host)
    FIELD(Required<quint16>, port)
};

struct RADAPTER_API TcpDevice : public ServerInfo
{
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(HasDefault<QString>, name)

    void postUpdate() override;
};

struct RADAPTER_API SerialDevice : Serializable
{
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<QString>, port_name)
    FIELD(HasDefault<QString>, name)
    FIELD(HasDefault<int>, parity, QSerialPort::NoParity)
    FIELD(HasDefault<int>, baud, QSerialPort::Baud115200)
    FIELD(HasDefault<int>, data_bits, QSerialPort::Data8)
    FIELD(HasDefault<int>, stop_bits, QSerialPort::OneStop)
    FIELD(NonRequiredByteOrder, byte_order, QDataStream::BigEndian)

    void postUpdate() override;
};


struct RADAPTER_API SqlClientInfo : ServerInfo
{
    typedef QMap<QString, SqlClientInfo> Map;
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<QString>, name)
    FIELD(Required<QString>, database)
    FIELD(Required<QString>, username)

    void postUpdate() override;
    static const SqlClientInfo &get(const QString& name);
};

struct RADAPTER_API SqlStorageInfo : Serializable
{
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<Worker>, worker)
    FIELD(Required<QString>, name)
    FIELD(Required<QString>, client_name)
    FIELD(Required<QString>, target_table)
    FIELD(Required<QString>, table_name)
};

struct RADAPTER_API LocalizationInfo : Serializable {

    Q_GADGET
    IS_SERIALIZABLE
    FIELD(RequiredTimeZone, time_zone)
};

struct RADAPTER_API WebsocketServer : Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<Worker>, worker)
    FIELD(HasDefault<QString>, bind_to, "0.0.0.0")
    FIELD(HasDefault<quint16>, port, 1234)
    FIELD(HasDefault<quint16>, heartbeat_ms, 10000)
    FIELD(HasDefault<quint16>, keepalive_time, 20000)
    FIELD(HasDefault<QString>, name, "redis-adapter")
    FIELD(HasDefault<bool>, secure, false)
    void postUpdate() override;
};

struct RADAPTER_API WebsocketClient : Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<Worker>, worker)
    FIELD(Required<QString>, host)
    FIELD(HasDefault<quint16>, port, 1234)
    FIELD(HasDefault<quint16>, heartbeat_ms, 10000)
    FIELD(HasDefault<quint16>, keepalive_time, 20000)
    FIELD(HasDefault<QString>, name, "redis-adapter")
    FIELD(HasDefault<bool>, secure, false)
};

} // namespace Settings

#endif // SETTINGS_H
