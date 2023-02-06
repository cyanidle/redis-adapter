#ifndef MODBUSSETTINGS_H
#define MODBUSSETTINGS_H

#include "radapterlogging.h"
#include "settings.h"
#include "jsondict/jsondict.hpp"
#include "broker/interceptors/logginginterceptor.h"

namespace Settings {

    struct RADAPTER_SHARED_SRC SerialDevice : public Serializer::SerializableGadget {
        typedef QMap<QString, SerialDevice> Map;
        static Map cacheMap;
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, name, "");
        SERIAL_FIELD(QString, port_name);
        SERIAL_FIELD(int, parity, QSerialPort::NoParity)
        SERIAL_FIELD(int, baud, QSerialPort::Baud115200)
        SERIAL_FIELD(int, data_bits, QSerialPort::Data8)
        SERIAL_FIELD(int, stop_bits, QSerialPort::OneStop)
        QString repr() const {
            return QStringLiteral("Serial dev %1; Port: %2; Parity: %3; Baud: %4; Data bits: %5; Stop bits: %6")
                .arg(name, port_name)
                .arg(parity, baud, data_bits)
                .arg(stop_bits);
        }

        SERIAL_POST_INIT(cache)
        void cache() {
            if (!name.isEmpty()) {
                cacheMap.insert(name, *this);
            }
        }

        SerialDevice()
            : parity(QSerialPort::NoParity),
              baud(QSerialPort::Baud115200),
              data_bits(QSerialPort::Data8),
              stop_bits(QSerialPort::OneStop)
        {
        }

        bool isValid() const {return !name.isEmpty() && !port_name.isEmpty();}

        bool operator==(const SerialDevice &src) const {
            return port_name == src.port_name
                && parity == src.parity
                && baud == src.baud
                && data_bits == src.data_bits
                && stop_bits == src.stop_bits;
        }
        bool operator!=(const SerialDevice &src)const {
            return !(*this == src);
        }
    };

    enum ModbusChannelType {
        MbMaster = 0,
        MbSlave
    };

    enum ModbusConnectionType {
        Serial = 0,
        Tcp,
        Unknown
    };

    struct RADAPTER_SHARED_SRC ModbusQuery : Serializer::SerializableGadget {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_CUSTOM(QModbusDataUnit::RegisterType, type, initType, readType)
        SERIAL_FIELD(quint16, reg_index)
        SERIAL_FIELD(quint8, reg_count)
        SERIAL_FIELD(quint16, poll_rate, 500)

        bool initType (const QVariant &src) {
            auto strRep = src.toString().toLower();
            if (strRep == "holding") {
                type = QModbusDataUnit::RegisterType::HoldingRegisters;
            } else if (strRep == "input") {
                type = QModbusDataUnit::RegisterType::InputRegisters;
            } else if (strRep == "coils") {
                type = QModbusDataUnit::RegisterType::Coils;
            } else if (strRep == "discrete_inputs") {
                type = QModbusDataUnit::RegisterType::DiscreteInputs;
            } else {
                auto errorMsg = QString("Incorrect RegisterType: ") + src.toString();
                throw std::runtime_error(errorMsg.toStdString());
            }
            return true;
        }
        QVariant readType () const {
            if (type == QModbusDataUnit::RegisterType::HoldingRegisters) {
                return "holding";
            } else if (type == QModbusDataUnit::RegisterType::InputRegisters) {
                return "input";
            } else if (type == QModbusDataUnit::RegisterType::Coils) {
                return "coils";
            } else if (type == QModbusDataUnit::RegisterType::DiscreteInputs) {
                return "discrete_inputs";
            } else {
                return QVariant();
            }
        }
    };

    struct RADAPTER_SHARED_SRC ModbusConnectionSource : Serializer::SerializableGadget {
        typedef QMap<QString, ModbusConnectionSource> Map;
        static Map cacheMap;
        Q_GADGET
        IS_SERIALIZABLE
        ModbusConnectionType type;
        SERIAL_FIELD(QString, name)
        SerialDevice serial;
        TcpDevice tcp;
        SERIAL_FIELD(int, response_time, 200)
        SERIAL_FIELD(int, number_of_retries, 3)
        SERIAL_POST_INIT(initDevice)
        void initDevice() {
            if (TcpDevice::cacheMap().contains(name)) {
                tcp = TcpDevice::cacheMap().value(name);
                type = ModbusConnectionType::Tcp;
            } else if (SerialDevice::cacheMap.contains(name)) {
                serial = SerialDevice::cacheMap.value(name);
                type = ModbusConnectionType::Serial;
            } else {
                throw std::runtime_error("Missing device: " + name.toStdString());
            }
        }
    };

    struct RADAPTER_SHARED_SRC ModbusSlaveInfo : Serializer::SerializableGadget {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(quint8, address)
        SERIAL_CUSTOM(ModbusConnectionSource, source, initSource, readSource)
        SERIAL_CONTAINER(QList, ModbusQuery, queries)
        SERIAL_CONTAINER(QList, QString, registers)

        bool initSource(const QVariant &src) {
            auto sourceName = src.toString();
            source.name = sourceName;
            if (TcpDevice::cacheMap().contains(sourceName)) {
                auto tcpDevice = TcpDevice::cacheMap().value(sourceName);
                source.type = ModbusConnectionType::Tcp;
                source.tcp = tcpDevice;
                return true;
            } else if (SerialDevice::cacheMap.contains(sourceName)) {
                auto serialDevice = SerialDevice::cacheMap.value(sourceName);
                source.type = ModbusConnectionType::Serial;
                source.serial = serialDevice;
                return true;
            }
            reError() << "Missing TCP/RTU Source with name:" << sourceName;
            return false;
        }
        QVariant readSource() const {
            return source.name;
        }
    };

    struct RADAPTER_SHARED_SRC ModbusChannelSettings : Serializer::SerializableGadget {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, name)
        SERIAL_FIELD(quint8, slave_address, 0)
        SERIAL_CONTAINER(QList, ModbusSlaveInfo, slaves, DEFAULT)
    };

    struct RADAPTER_SHARED_SRC PackingMode : Serializer::SerializableGadget {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_CUSTOM(QDataStream::ByteOrder, byte_order, initByteOrder, readByteOrder, QDataStream::BigEndian)
        SERIAL_CUSTOM(QDataStream::ByteOrder, word_order, initWordOrder, readWordOrder, QDataStream::LittleEndian)
        static QDataStream::ByteOrder getOrder(const QString &name) {
            if (name.toLower() == "littleendian") {
                return QDataStream::LittleEndian;
            } else if (name.toLower() == "bigendian") {
                return QDataStream::BigEndian;
            } else {
                auto msg = QString("Error endianess passed: ") + name;
                throw std::runtime_error(msg.toStdString());
            }
        }
        static QString read_order(QDataStream::ByteOrder order) {
            if (order == QDataStream::LittleEndian) {
                return "littleendian";
            } else {
                return "bigendian";
            }
        }
        bool initByteOrder(const QVariant &src) {
            byte_order = getOrder(src.toString());
            return true;
        }
        QVariant readByteOrder() const {
            return read_order(byte_order);
        }
        bool initWordOrder(const QVariant &src) {
            word_order = getOrder(src.toString());
            return true;
        }
        QVariant readWordOrder() const {
            return read_order(word_order);
        }
        bool operator==(const PackingMode &src) const{
            if (
                byte_order == src.byte_order
                && word_order == src.word_order
            )
                return true;
            else
                return false;
        }
        bool operator !=(const PackingMode &src) const{
            return !(*this == src);
        }
    };

    struct RADAPTER_SHARED_SRC RegisterInfo : Serializer::SerializableGadget {
        typedef QMap<QString, QModbusDataUnit::RegisterType> tableMap;
        typedef QMap<QString, QMetaType::Type> typeMap;
        static typeMap stringToType() {
            return {{"uint16", QMetaType::UShort},
                    {"uint32", QMetaType::UInt},
                    {"float", QMetaType::Float},
                    {"float32", QMetaType::Float}};}
        static tableMap stringToTable() {
            return {{"holding",QModbusDataUnit::RegisterType::HoldingRegisters},
                    {"input",QModbusDataUnit::RegisterType::InputRegisters},
                    {"coils",QModbusDataUnit::RegisterType::Coils},
                    {"discrete_inputs",QModbusDataUnit::RegisterType::DiscreteInputs}};}
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_CUSTOM(QModbusDataUnit::RegisterType, table, initTable, readTable)
        SERIAL_FIELD(quint16, index)
        SERIAL_CUSTOM(QMetaType::Type, type, initType, readType, QMetaType::UShort)
        SERIAL_FIELD(PackingMode, endianess, DEFAULT)
        SERIAL_FIELD(bool, is_persistent, false)

        bool isValid() const {
            return table != QModbusDataUnit::Invalid;
        }

        bool initType(const QVariant &src) {
            type = stringToType().value(src.toString(), QMetaType::UnknownType);
            if (QMetaType::sizeOf(type) < 2) {
                throw std::runtime_error("Types with size less than 2 bytes (1 word) are not supported!");
            }
            return type != QMetaType::UnknownType;
        }

        QVariant readType() const {
            return stringToType().key(type);
        }

        bool initTable(const QVariant &src) {
            table = stringToTable().value(src.toString());
            return table != QModbusDataUnit::Invalid;
        }

        QVariant readTable() const {
            return stringToTable().key(table);
        }

        bool operator==(const RegisterInfo &src) const{
            return table == src.table
                && index == src.index
                && type == src.type
                && endianess == src.endianess
                && is_persistent == src.is_persistent;
        }

        bool operator!=(const RegisterInfo &other) {
            return !(*this == other);
        }
    };

    typedef QMultiMap<QString /*regName*/, RegisterInfo> DeviceRegistersInfo;
    typedef QMap<QString /*deviceName*/, DeviceRegistersInfo> DeviceRegistersInfoMap;

    struct RADAPTER_SHARED_SRC DeviceRegistersInfoMapParser {
        static DeviceRegistersInfoMap parse(const QVariant &src);
    };

    struct RADAPTER_SHARED_SRC ModbusTcpDevicesSettings : Serializer::SerializableGadget {
        Q_GADGET
        IS_SERIALIZABLE
         SERIAL_CONTAINER_PTRS(QList, TcpDevice, devices);
    };

    struct RADAPTER_SHARED_SRC ModbusRtuDevicesSettings : Serializer::SerializableGadget {
        Q_GADGET
        IS_SERIALIZABLE
         SERIAL_CONTAINER_PTRS(QList, SerialDevice, devices);
    };

    struct RADAPTER_SHARED_SRC ModbusConnectionSettings : Serializer::SerializableGadget {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_CONTAINER(QList, ModbusChannelSettings, channels);
        SerialDevice::Map serial_devices;
        TcpDevice::Map tcp_devices;
        SERIAL_FIELD(quint16, poll_rate, 500)
        SERIAL_FIELD(quint16, response_time, 150)
        SERIAL_FIELD(quint16, retries, 3)
        SERIAL_FIELD(bool, debug, false)
        SERIAL_FIELD_PTR(Radapter::LoggingInterceptorSettings, log_jsons, DEFAULT)
        SERIAL_FIELD(Radapter::WorkerSettings, worker)
        SERIAL_FIELD(ModbusTcpDevicesSettings, tcp, DEFAULT)
        SERIAL_FIELD(ModbusRtuDevicesSettings, rtu, DEFAULT)
        SERIAL_FIELD(QString, filter_name, DEFAULT)
        SERIAL_POST_INIT(postInit)
        void postInit() {
            for (auto &device : rtu.devices) {
                serial_devices.insert(device->name, *device);
            }
            for (auto &device : tcp.devices) {
                tcp_devices.insert(device->name, *device);
            }
            for (auto &channel : channels) {
                for (auto &slave: channel.slaves) {
                    slave.source.number_of_retries = retries;
                    slave.source.response_time = response_time;
                    for (auto &query : slave.queries) {
                        query.poll_rate = poll_rate;
                    }
                }
            }
        }
        bool isValid() const {
            return worker.isValid();
        }
    };

    struct RADAPTER_SHARED_SRC ModbusDevice : Serializer::SerializableGadget {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD_PTR(TcpDevice, tcp, DEFAULT)
        SERIAL_FIELD_PTR(SerialDevice, rtu, DEFAULT)
        ModbusConnectionType device_type = ModbusConnectionType::Unknown;
        SERIAL_POST_INIT(postInit)
        QString repr() const {
            return device_type==ModbusConnectionType::Serial?
                rtu->repr():
                tcp->repr();
        }
        void postInit() {
            if (tcp && rtu) {
                throw std::runtime_error("Both tcp and rtu device is prohibited! Use one");
            }
            if (tcp) {
                device_type = Tcp;
            } else if (rtu) {
                device_type = Serial;
            } else {
                throw std::runtime_error("Provide at least one target device!");
            }
        }
    };

    struct RADAPTER_SHARED_SRC ModbusSlaveWorker : Serializer::SerializableGadget {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(Radapter::WorkerSettings, worker)
        SERIAL_FIELD(ModbusDevice, device)
        SERIAL_FIELD(quint32, reconnect_timeout_ms, 5000)
        SERIAL_FIELD(quint16, slave_id)
        SERIAL_CUSTOM(quint16, holding_registers, parseHolding, NO_READ, DEFAULT)
        SERIAL_CUSTOM(quint16, input_registers, parseInput, NO_READ, DEFAULT)
        SERIAL_CUSTOM(quint16, di, parseDi, NO_READ, DEFAULT)
        SERIAL_CUSTOM(quint16, coils, parseCoils, NO_READ, DEFAULT)
        DeviceRegistersInfo registers = {};
        SERIAL_POST_INIT(postInit)
        void postInit() const;
        bool parseHolding(const QVariant &src);
        bool parseInput(const QVariant &src);
        bool parseDi(const QVariant &src);
        bool parseCoils(const QVariant &src);
        static DeviceRegistersInfo parseRegisters(const JsonDict &target);
    };


}

#endif // MODBUSSETTINGS_H
