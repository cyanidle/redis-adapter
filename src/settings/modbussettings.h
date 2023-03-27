#ifndef MODBUSSETTINGS_H
#define MODBUSSETTINGS_H

#include "broker/sync/channel.h"
#include "qstringbuilder.h"
#include "qthread.h"
#include "settings.h"
#include "broker/interceptors/logginginterceptor.h"

Q_DECLARE_METATYPE(QMetaType::Type)

namespace Settings {
    struct ChooseRegValueType {
        static bool validate(QVariant& value) {
            static QMap<QString, QMetaType::Type>
                    map{{"uint16", QMetaType::UShort},
                        {"uint32", QMetaType::UInt},
                        {"float", QMetaType::Float},
                        {"float32", QMetaType::Float}};
            auto asStr = value.toString().toLower();
            value.setValue(map.value(asStr));
            return map.contains(asStr);
        }
    };
    struct ChooseRegisterTable {
        static bool validate(QVariant& value) {
            static QMap<QString, QModbusDataUnit::RegisterType>
                map{{"holding",QModbusDataUnit::HoldingRegisters},
                    {"input",QModbusDataUnit::InputRegisters},
                    {"coils",QModbusDataUnit::Coils},
                    {"discrete_inputs",QModbusDataUnit::DiscreteInputs}};
            auto asStr = value.toString().toLower();
            value.setValue(map.value(asStr));
            return map.contains(asStr);
        }
    };
    using RegisterTable = Serializable::Validate<RequiredField<QModbusDataUnit::RegisterType>, ChooseRegisterTable>;
    using RegisterValueType = Serializable::Validate<RequiredField<QMetaType::Type>, ChooseRegValueType>;

    struct RADAPTER_API ModbusQuery : SerializableSettings {
        Q_GADGET
        FIELDS(type, reg_index, reg_count)
        RegisterTable type;
        RequiredField<quint16> reg_index;
        RequiredField<quint8> reg_count;
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
    struct RADAPTER_API PackingMode : SerializableSettings {
        Q_GADGET
        FIELDS(byte_order, word_order)
        NonRequiredByteOrder byte_order{QDataStream::LittleEndian};
        NonRequiredByteOrder word_order{QDataStream::LittleEndian};
    };
    struct RADAPTER_API ModbusDevice : SerializableSettings {
        typedef QMap<QString, ModbusDevice> Map;
        Q_GADGET
        FIELDS(tcp, rtu)
        NonRequiredField<TcpDevice> tcp;
        NonRequiredField<SerialDevice> rtu;

        QSharedPointer<Radapter::Sync::Channel> channel;

        static const ModbusDevice &get(const QString &name) {
            if (!table().contains(name)) throw std::invalid_argument("Missing Modbus Device: " + name.toStdString());
            return table()[name];
        }
        QString repr() const {
            if (tcp->port) {
                return QStringLiteral("TCP: Host:%1 Port:%2").arg(tcp->host.value).arg(tcp->port.value);
            } else if (!rtu->port_name->isEmpty()) {
                return QStringLiteral("RTU: Port:%1 Baud:%2").arg(rtu->port_name.value).arg(rtu->baud.value);
            } else {
                return "Invalid";
            }
        }
    protected:
        static Map &table() {
            static Map map{};
            return map;
        }
        POST_UPDATE {
            static QThread channelsThread;
            if (!channel) channel.reset(new Radapter::Sync::Channel(&channelsThread));
            channelsThread.start();
            if (tcp.value && rtu.value) throw std::runtime_error("[Modbus Device] Both tcp and rtu device is prohibited! Use one");
            if (!tcp.value && !rtu.value) throw std::runtime_error("[Modbus Device] Tcp or Rtu device not specified!");
            table().insert(tcp.value ? tcp->name : rtu->name, *this);
        }
    };

    struct RADAPTER_API RegisterInfo : SerializableSettings {
        typedef QMap<QString, QModbusDataUnit::RegisterType> tableMap;
        typedef QMap<QString, QMetaType::Type> typeMap;
        Q_GADGET
        FIELDS(index, resetting, table, type)
        RegisterTable table;
        RequiredField<int> index;

        NonRequiredField<bool> resetting{false};
        MarkNonRequired<RegisterValueType> type{QMetaType::UShort};
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

    struct RADAPTER_API ModbusSlave : SerializableSettings {
        Q_GADGET
        FIELDS(worker, device_name, reconnect_timeout_ms, slave_id, endianess, register_names)
        RequiredField<Radapter::WorkerSettings> worker;
        RequiredField<QString> device_name;
        RequiredField<quint16> slave_id;
        RequiredSequence<QString> register_names;
        NonRequiredField<quint32> reconnect_timeout_ms{5000};
        NonRequiredField<PackingMode> endianess;

        ModbusDevice device{};
        RegisterCounts counts{};
        Registers registers{};

        void postUpdate() override;
    };

    struct RADAPTER_API ModbusMaster : SerializableSettings {
        Q_GADGET
        FIELDS(worker, device_name, slave_id, queries, register_names)
        RequiredField<Radapter::WorkerSettings> worker;
        RequiredField<QString> device_name;
        RequiredField<quint16> slave_id;
        RequiredSequence<ModbusQuery> queries;
        RequiredSequence<QString> register_names;

        NonRequiredField<quint32> poll_rate{500},
                                  reconnect_timeout_ms{5000},
                                  responce_time{150},
                                  retries{3};
        NonRequiredField<PackingMode> endianess;
        ModbusDevice device{};
        Registers registers{};
        void postUpdate() override;
    };

}

#endif // MODBUSSETTINGS_H
