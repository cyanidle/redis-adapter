#ifndef MODBUSSETTINGS_H
#define MODBUSSETTINGS_H

#include "broker/sync/channel.h"
#include "qthread.h"
#include "settings.h"

Q_DECLARE_METATYPE(QMetaType::Type)

namespace Settings {
    struct ChooseRegValueType {
        static bool validate(QVariant& value) {
            static QMap<QString, QMetaType::Type>
                    map{{"uint16", QMetaType::UShort},
                        {"word", QMetaType::UShort},
                        {"uint32", QMetaType::UInt},
                        {"dword", QMetaType::UInt},
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
                    {"discrete_inputs",QModbusDataUnit::DiscreteInputs},
                    {"holding_registers",QModbusDataUnit::HoldingRegisters},
                    {"input_registers",QModbusDataUnit::InputRegisters},
                    {"coils",QModbusDataUnit::Coils},
                    {"di",QModbusDataUnit::DiscreteInputs},
                    {"do",QModbusDataUnit::Coils},};
            auto asStr = value.toString().toLower();
            value.setValue(map.value(asStr));
            return map.contains(asStr);
        }
    };
    using RegisterTable = Serializable::Validate<Required<QModbusDataUnit::RegisterType>, ChooseRegisterTable>;
    using RegisterValueType = Serializable::Validate<Required<QMetaType::Type>, ChooseRegValueType>;

    struct RADAPTER_API ModbusQuery : SerializableSettings {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(RegisterTable, type)
        FIELD(Required<quint16>, reg_index)
        FIELD(Required<quint8>, reg_count)
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
        IS_SERIALIZABLE
        FIELD(NonRequiredByteOrder, bytes, {QDataStream::LittleEndian})
        FIELD(NonRequiredByteOrder, words, {QDataStream::LittleEndian})
        PackingMode() = default;
        PackingMode(QDataStream::ByteOrder words, QDataStream::ByteOrder bytes) :
            bytes(bytes), words(words)
        {}
    };

    struct RADAPTER_API ModbusDevice : SerializableSettings {
        typedef QMap<QString, ModbusDevice> Map;
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(NonRequired<TcpDevice>, tcp)
        FIELD(NonRequired<SerialDevice>, rtu)

        QSharedPointer<Radapter::Sync::Channel> channel;

        static const ModbusDevice &get(const QString &name) {
            if (!table().contains(name)) throw std::invalid_argument("Missing Modbus Device: " + name.toStdString());
            return table()[name];
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

    struct OrdersValidator {
        static bool validate(QVariant &value);
    };

    struct RADAPTER_API RegisterInfo : SerializableSettings {
        typedef QMap<QString, QModbusDataUnit::RegisterType> tableMap;
        typedef QMap<QString, QMetaType::Type> typeMap;
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(RegisterTable, table)
        FIELD(Required<int>, index)
        FIELD(NonRequired<bool>, resetting, {false})
        FIELD(MarkNonRequired<RegisterValueType>, type, {QMetaType::UShort})
        using Orders = Serializable::Validate<NonRequired<PackingMode>, OrdersValidator>;
        FIELD(Orders, order)
    };
    typedef QMap<QString /*regName*/, RegisterInfo> Registers;
    typedef QMap<QString /*deviceName*/, Registers> DevicesRegisters;
    void RADAPTER_API parseRegisters(const QVariant &registersFile);

    struct RegisterCounts {
        quint16 coils{};
        quint16 di{};
        quint16 holding_registers{};
        quint16 input_registers{};
    };

    struct RADAPTER_API ModbusWorker : SerializableSettings {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(Required<Radapter::WorkerSettings>, worker)
        FIELD(Required<QString>, device_name)
        FIELD(Required<quint16>, slave_id)
        FIELD(RequiredSequence<QString>, register_names)
    };

    struct RADAPTER_API ModbusSlave : ModbusWorker {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(NonRequired<quint32>, reconnect_timeout_ms, {5000})

        ModbusDevice device{};
        RegisterCounts counts{};
        Registers registers{};

        void postUpdate() override;
    };

    struct RADAPTER_API ModbusMaster : ModbusWorker {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(RequiredSequence<ModbusQuery>, queries)

        FIELD(NonRequired<quint32>, poll_rate, {500})
        FIELD(NonRequired<quint32>, reconnect_timeout_ms, {5000})
        FIELD(NonRequired<quint32>, responce_time, {150})
        FIELD(NonRequired<quint32>, retries, {3})

        ModbusDevice device{};
        Registers registers{};
        void postUpdate() override;
    };

}

#endif // MODBUSSETTINGS_H
