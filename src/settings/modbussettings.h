#ifndef MODBUSSETTINGS_H
#define MODBUSSETTINGS_H

#include "broker/sync/channel.h"
#include "qstringbuilder.h"
#include "qthread.h"
#include "settings.h"
#include "broker/interceptors/logginginterceptor.h"

Q_DECLARE_METATYPE(QMetaType::Type)
Q_DECLARE_METATYPE(QDataStream::ByteOrder)

namespace Settings {

    struct RADAPTER_SHARED_SRC ModbusQuery : SerializableSettings {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD_MAPPED(QModbusDataUnit::RegisterType, type, table())
        SERIAL_FIELD(quint16, reg_index)
        SERIAL_FIELD(quint8, reg_count)
        typedef QMap<QString, QModbusDataUnit::RegisterType> Map;
    protected:
        static Map &table() {
            static Map map{
                {"holding", QModbusDataUnit::HoldingRegisters},
                {"input", QModbusDataUnit::InputRegisters},
                {"coils", QModbusDataUnit::Coils},
                {"discrete_inputs", QModbusDataUnit::DiscreteInputs}
            };
            return map;
        }
    };
    struct RADAPTER_SHARED_SRC PackingMode : SerializableSettings {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD_MAPPED(QDataStream::ByteOrder, byte_order, table(), QDataStream::LittleEndian)
        SERIAL_FIELD_MAPPED(QDataStream::ByteOrder, word_order, table(), QDataStream::LittleEndian)
        typedef QMap<QString, QDataStream::ByteOrder> Map;
    protected:
        static Map &table() {
            static Map map{
                {"littleendian",  QDataStream::LittleEndian},
                {"bigendian", QDataStream::BigEndian}
            };
            return map;
        }
    };
    struct RADAPTER_SHARED_SRC ModbusDevice : SerializableSettings {
        typedef QMap<QString, ModbusDevice> Map;
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD_PTR(TcpDevice, tcp, DEFAULT)
        SERIAL_FIELD_PTR(SerialDevice, rtu, DEFAULT)
        QSharedPointer<Radapter::Sync::Channel> channel;
        SERIAL_POST_INIT(postInit)
        static const ModbusDevice &get(const QString &name) {
            if (!table().contains(name)) throw std::invalid_argument("Missing Modbus Device: " + name.toStdString());
            return table()[name];
        }
        QString repr() const {
            if (tcp) {
                return QStringLiteral("TCP: Host:%1 Port:%2").arg(tcp->host).arg(tcp->port);
            } else if (rtu) {
                return QStringLiteral("RTU: Port:%1 Baud:%2").arg(rtu->port_name).arg(rtu->baud);
            } else {
                return "Invalid";
            }
        }
    protected:
        static Map &table() {
            static Map map{};
            return map;
        }
        void postInit() {
            static QThread channelsThread;
            if (!channel) channel.reset(new Radapter::Sync::Channel(&channelsThread));
            channelsThread.start();
            if (tcp && rtu) throw std::runtime_error("Both tcp and rtu device is prohibited! Use one");
            if (!tcp && !rtu) throw std::runtime_error("Tcp or Rtu device not specified or 'source' not found!");
            table().insert(tcp ? tcp->name : rtu->name, *this);
        }
    };
    struct RADAPTER_SHARED_SRC RegisterInfo : SerializableSettings {
        typedef QMap<QString, QModbusDataUnit::RegisterType> tableMap;
        typedef QMap<QString, QMetaType::Type> typeMap;
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD_MAPPED(QModbusDataUnit::RegisterType, table, stringToTable())
        SERIAL_FIELD(int, index)
        SERIAL_FIELD_MAPPED(QMetaType::Type, type, stringToType(), QMetaType::UShort)
    protected:
        static typeMap stringToType() {
            return {{"uint16", QMetaType::UShort},
                    {"uint32", QMetaType::UInt},
                    {"float", QMetaType::Float},
                    {"float32", QMetaType::Float}};}
        static tableMap stringToTable() {
            return {{"holding",QModbusDataUnit::HoldingRegisters},
                    {"input",QModbusDataUnit::InputRegisters},
                    {"coils",QModbusDataUnit::Coils},
                    {"discrete_inputs",QModbusDataUnit::DiscreteInputs}};}
    };
    typedef QMap<QString /*regName*/, RegisterInfo> Registers;
    typedef QMap<QString /*deviceName*/, Registers> DevicesRegisters;
    void parseRegisters(const QVariant &registersFile);

    struct RegisterCounts {
        quint16 coils{};
        quint16 di{};
        quint16 holding_registers{};
        quint16 input_registers{};
    };

    struct RADAPTER_SHARED_SRC ModbusSlave : SerializableSettings {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(Radapter::WorkerSettings, worker)
        SERIAL_FIELD(QString, device_name)
        SERIAL_FIELD(quint32, reconnect_timeout_ms, 5000)
        SERIAL_FIELD(quint16, slave_id)
        SERIAL_FIELD(PackingMode, endianess, DEFAULT)
        SERIAL_CONTAINER(QList, QString, register_names)

        ModbusDevice device{};
        RegisterCounts counts{};
        Registers registers{};

        SERIAL_POST_INIT(postInit)
        void postInit();
    };

    struct RADAPTER_SHARED_SRC ModbusMaster : SerializableSettings {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(Radapter::WorkerSettings, worker)
        SERIAL_FIELD(QString, device_name)
        SERIAL_FIELD(quint16, slave_id)
        SERIAL_CONTAINER(QList, ModbusQuery, queries)
        SERIAL_CONTAINER(QList, QString, register_names)

        SERIAL_FIELD(quint32, poll_rate, 500)
        SERIAL_FIELD(quint32, reconnect_timeout_ms, 5000)
        SERIAL_FIELD(quint32, responce_time, 150)
        SERIAL_FIELD(quint32, retries, 3)
        SERIAL_FIELD(PackingMode, endianess, DEFAULT)
        SERIAL_FIELD_PTR(Radapter::LoggingInterceptorSettings, log_jsons, DEFAULT)

        ModbusDevice device{};
        Registers registers{};

        SERIAL_POST_INIT(postInit)
        void postInit();
    };

}

#endif // MODBUSSETTINGS_H
