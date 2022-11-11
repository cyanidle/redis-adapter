#ifndef MODBUSSETTINGS_H
#define MODBUSSETTINGS_H

#include "settings.h"
#include "JsonFormatters"

namespace Settings {

    struct RADAPTER_SHARED_SRC SerialDevice : public Serializer::SerializerBase {
        typedef QMap<QString, SerialDevice> Map;
        static Map cacheMap;
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, name);
        SERIAL_FIELD(QString, port_name);
        SERIAL_FIELD(int, parity)
        SERIAL_FIELD(int, baud)
        SERIAL_FIELD(int, data_bits)
        SERIAL_FIELD(int, stop_bits)
        SERIAL_POST_INIT(cache)
        void cache() {
            cacheMap.insert(name, *this);
        }

        SerialDevice()
            : parity(QSerialPort::NoParity),
              baud(QSerialPort::Baud115200),
              data_bits(QSerialPort::Data8),
              stop_bits(QSerialPort::OneStop)
        {
        }

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

    struct RADAPTER_SHARED_SRC ModbusQuery : Serializer::SerializerBase {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_CUSTOM(QModbusDataUnit::RegisterType, type, initType, readType)
        SERIAL_FIELD(quint16, reg_index)
        SERIAL_FIELD(quint8, reg_count)
        SERIAL_FIELD(quint16, poll_rate, 0)

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
        bool operator==(const ModbusQuery &src)const {
            return reg_index == src.reg_index
                && type == src.type
                && reg_count == src.reg_count
                && poll_rate == src.poll_rate;
        }
        bool operator!=(const ModbusQuery &src)const {
            return !(*this == src);
        }
    };

    struct RADAPTER_SHARED_SRC ModbusConnectionSource : Serializer::SerializerBase {
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
            if (TcpDevice::cacheMap.contains(name)) {
                tcp = TcpDevice::cacheMap.value(name);
                type = ModbusConnectionType::Tcp;
            } else if (SerialDevice::cacheMap.contains(name)) {
                serial = SerialDevice::cacheMap.value(name);
                type = ModbusConnectionType::Serial;
            } else {
                type = ModbusConnectionType::Unknown;
            }
        }

        bool isValid() {
            return !serial.port_name.isEmpty()
                    || (!tcp.ip.isEmpty()
                        && tcp.port != 0u);
        }
        bool operator==(const ModbusConnectionSource &src) const {
            return name == src.name
                && type == src.type
                && serial == src.serial
                && tcp == src.tcp
                && response_time == src.response_time
                && number_of_retries == src.number_of_retries;
        }
        bool operator!=(const ModbusConnectionSource &src)const {
            return !(*this == src);
        }
    };

    struct RADAPTER_SHARED_SRC ModbusSlaveInfo : Serializer::SerializerBase {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(quint8, address)
        SERIAL_CUSTOM(ModbusConnectionSource, source, initSource, readSource)
        SERIAL_CONTAINER_NEST(QList, ModbusQuery, queries)
        SERIAL_CONTAINER(QList, QString, registers)

        bool initSource(const QVariant &src) {
            auto sourceName = src.toString();
            source.name = sourceName;
            if (TcpDevice::cacheMap.contains(sourceName)) {
                auto tcpDevice = TcpDevice::cacheMap.value(sourceName);
                source.type = ModbusConnectionType::Tcp;
                source.tcp = tcpDevice;
                return true;
            } else if (SerialDevice::cacheMap.contains(sourceName)) {
                auto serialDevice = SerialDevice::cacheMap.value(sourceName);
                source.type = ModbusConnectionType::Serial;
                source.serial = serialDevice;
                return true;
            }
            return false;
        }

        QVariant readSource() const {
            return source.name;
        }

        bool isValid() {
            return address != 0u
                    && !queries.isEmpty()
                    && source.isValid();
        }
        bool operator==(const ModbusSlaveInfo &src)const {
            return address == src.address
                && source == src.source
                && queries == src.queries
                && registers == src.registers;
        }
        bool operator!=(const ModbusSlaveInfo &src)const {
            return !(*this == src);
        }
    };

    struct RADAPTER_SHARED_SRC ModbusChannelSettings : Serializer::SerializerBase {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, name)
        SERIAL_CUSTOM(ModbusChannelType, type, initChannelType, readChannelType)
        SERIAL_FIELD(quint8, slave_address, 0)
        SERIAL_CONTAINER_NEST(QList, ModbusSlaveInfo, slaves, {})
        bool initChannelType(const QVariant &src) {
            auto rawType = src.toString();
            if (rawType == "slave") {
                type = ModbusChannelType::MbSlave;
                return true;
            }
            if (rawType == "master") {
                type = ModbusChannelType::MbMaster;
                return true;
            }
            return false;
        }

        QVariant readChannelType() const {
            return QVariant(type);
        }

        bool operator==(const ModbusChannelSettings &src)const {
            return name == src.name
                && type == src.type
                && slave_address == src.slave_address
                && slaves == src.slaves;
        }
        bool operator!=(const ModbusChannelSettings &src)const {
            return !(*this == src);
        }
    };

    struct RADAPTER_SHARED_SRC PackingMode : Serializer::SerializerBase {
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

    struct RADAPTER_SHARED_SRC RegisterInfo : Serializer::SerializerBase {

        typedef QMap<QString, QModbusDataUnit::RegisterType> tableMap;
        typedef QMap<QString, QMetaType::Type> typeMap;
        static tableMap StringToTable;
        static typeMap StringToType;

        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_CUSTOM(QModbusDataUnit::RegisterType, table, initTable, readTable)
        SERIAL_FIELD(quint16, index)
        SERIAL_CUSTOM(QMetaType::Type, type, initType, readType)
        SERIAL_NEST(PackingMode, endianess)
        SERIAL_FIELD(bool, is_persistent, false)

        bool isValid() const {
            return table != QModbusDataUnit::Invalid;
        }

        bool initType(const QVariant &src) {
            type = StringToType.value(src.toString());
            return true;
        }

        QVariant readType() const {
            return StringToType.key(type);
        }

        bool initTable(const QVariant &src) {
            table = StringToTable.value(src.toString());
            return true;
        }

        QVariant readTable() const {
            return StringToTable.key(table);
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

    struct RADAPTER_SHARED_SRC ModbusTcpDevicesSettings : Serializer::SerializerBase {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_CONTAINER_NEST(QList, TcpDevice, devices);
    };

    struct RADAPTER_SHARED_SRC ModbusRtuDevicesSettings : Serializer::SerializerBase {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_CONTAINER_NEST(QList, SerialDevice, devices);
    };

    struct RADAPTER_SHARED_SRC ModbusConnectionSettings : Serializer::SerializerBase {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_CONTAINER_NEST(QList, ModbusChannelSettings, channels);
        SerialDevice::Map serial_devices;
        TcpDevice::Map tcp_devices;
        SERIAL_FIELD(quint16, poll_rate, 500)
        SERIAL_FIELD(quint16, response_time, 150)
        SERIAL_FIELD(quint16, retries, 3)
        SERIAL_FIELD(bool, debug, false)
        SERIAL_CONTAINER(QList, QString, producers, DEFAULT)
        SERIAL_CONTAINER(QList, QString, consumers, DEFAULT)
        SERIAL_NEST(RecordOutgoingSetting, log_jsons, DEFAULT)
        SERIAL_NEST(ModbusTcpDevicesSettings, tcp, DEFAULT)
        SERIAL_NEST(ModbusRtuDevicesSettings, rtu, DEFAULT)
        SERIAL_FIELD(QString, filter_name, DEFAULT)
        Filters::Table filters;

        SERIAL_POST_INIT(postInit)

        void postInit() {
            for (auto &device : rtu.devices) {
                serial_devices.insert(device.name, device);
            }
            for (auto &device : tcp.devices) {
                tcp_devices.insert(device.name, device);
            }
            for (auto &channel : channels) {
                for (auto &slave: channel.slaves) {
                    slave.source.number_of_retries = retries;
                    slave.source.response_time = response_time;
                }
            }
            filters = Filters::tableMap.value(filter_name);
        }

        bool operator==(const ModbusConnectionSettings &src) const{
            return channels == channels
                && producers == src.producers
                && consumers == src.consumers;
        }

        bool operator!=(const ModbusConnectionSettings &other) {
            return !(*this == other);
        }
    };




}

#endif // MODBUSSETTINGS_H
