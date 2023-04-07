#ifndef MODBUSSETTINGS_H
#define MODBUSSETTINGS_H

#include "broker/sync/channel.h"
#include "consumers/rediscacheconsumer.h"
#include "producers/rediscacheproducer.h"
#include "qthread.h"
#include "settings.h"
#include "broker/workers/worker_field.hpp"

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
    };
    struct RADAPTER_API PackingMode : SerializableSettings {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(NonRequiredByteOrder, words, QDataStream::LittleEndian)
        FIELD(NonRequiredByteOrder, bytes, QDataStream::LittleEndian)
        PackingMode() = default;
        PackingMode(QDataStream::ByteOrder words, QDataStream::ByteOrder bytes) :
            words(words), bytes(bytes)
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
            settingsParsingWarn().noquote() << "New " << print();
            if (tcp->isValid() && rtu->isValid()) {
                throw std::runtime_error("[Modbus Device] Both tcp and rtu device is prohibited! Use one");
            } else if (!tcp->isValid() && !rtu->isValid()) {
                throw std::runtime_error("[Modbus Device] Tcp or Rtu device not specified!");
            }
            table().insert(tcp->port ? tcp->name : rtu->name, *this);
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
        FIELD(NonRequired<bool>, resetting, false)
        FIELD(MarkNonRequired<RegisterValueType>, type, QMetaType::UShort)
        using Orders = Serializable::Validate<NonRequired<PackingMode>, OrdersValidator>;
        FIELD(Orders, endianess)
    };
    typedef QMap<QString /*reg:Name*/, RegisterInfo> Registers;
    typedef QMap<QString /*deviceName*/, Registers> DevicesRegisters;
    void RADAPTER_API parseRegisters(const QVariantMap &registersFile);

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
        FIELD(NonRequired<quint32>, reconnect_timeout_ms, 5000)

        ModbusDevice device{};
        RegisterCounts counts{};
        Registers registers{};

        void postUpdate() override;
    };

    struct RADAPTER_API ModbusMaster : ModbusWorker {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(RequiredSequence<ModbusQuery>, queries)

        FIELD(NonRequired<quint32>, poll_rate, 500)
        FIELD(NonRequired<quint32>, reconnect_timeout_ms, 5000)
        FIELD(NonRequired<quint32>, responce_time, 150)
        FIELD(NonRequired<quint32>, retries, 3)

        FIELD(NonRequired<bool>, reliable_mode, false)
        FIELD(NonRequired<quint32>, rewrite_timeout_ms, 3000)
        FIELD(WorkerByNameNonRequired<Redis::CacheProducer*>, state_writer)
        FIELD(WorkerByNameNonRequired<Redis::CacheConsumer*>, state_reader)

        ModbusDevice device{};
        Registers registers{};
        void postUpdate() override;
    };

}

#endif // MODBUSSETTINGS_H
