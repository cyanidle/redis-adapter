#ifndef MODBUSSETTINGS_H
#define MODBUSSETTINGS_H

#include "broker/sync/channel.h"
#include "qthread.h"
#include "settings.h"

Q_DECLARE_METATYPE(QMetaType::Type)

namespace Settings {
    struct ChooseRegValueType {
        static bool validate(QVariant& value, const QVariantList &args, QVariant &state);
    };
    struct ChooseRegisterTable {
        static bool validate(QVariant& value, const QVariantList &args, QVariant &state);
    };
    using RequiredRegisterTable = Serializable::Validated<Required<QModbusDataUnit::RegisterType>>::With<ChooseRegisterTable>;
    using RegisterValueType = Serializable::Validated<Required<QMetaType::Type>>::With<ChooseRegValueType>;

    struct RADAPTER_API ModbusQuery : SerializableSettings {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(RequiredRegisterTable, type)
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
        FIELD(HasDefault<TcpDevice>, tcp)
        FIELD(HasDefault<SerialDevice>, rtu)

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
        static bool validate(QVariant &value, const QVariantList &args, QVariant &state);
    };

    struct RADAPTER_API RegisterInfo : SerializableSettings {
        Q_GADGET
        IS_SERIALIZABLE
        using Orders = Serializable::Validated<HasDefault<PackingMode>>::With<OrdersValidator>;
        FIELD(Orders, endianess)
        FIELD(RequiredRegisterTable, table)
        FIELD(Required<int>, index)
        FIELD(MarkHasDefault<RegisterValueType>, type, QMetaType::UShort)
        FIELD(HasDefault<bool>, resetting, false) // not implemented yet
        FIELD(HasDefault<bool>, writable, true)
        FIELD(OptionalValidator, validator)
        void postUpdate() override;
    };
    typedef QMap<QString /*reg:Name*/, RegisterInfo> Registers;
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
        FIELD(Required<Worker>, worker)
        FIELD(Required<QString>, device_name)
        FIELD(Required<quint16>, slave_id)
        FIELD(RequiredSequence<QString>, register_names)
        FIELD(HasDefault<quint32>, reconnect_timeout_ms, 3000)
    };

    struct RADAPTER_API ModbusSlave : ModbusWorker {
        Q_GADGET
        IS_SERIALIZABLE

        ModbusDevice device{};
        RegisterCounts counts{};
        Registers registers{};

        void init();
    };

    struct RADAPTER_API ModbusMaster : ModbusWorker {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(RequiredSequence<ModbusQuery>, queries)

        FIELD(HasDefault<quint32>, poll_rate, 500)
        FIELD(HasDefault<quint32>, response_time_ms, 150)
        FIELD(HasDefault<quint32>, retries, 3)

        FIELD(Optional<QString>, state_writer)
        FIELD(Optional<QString>, state_reader)

        ModbusDevice device{};
        Registers registers{};
        void init();
    };

}

#endif // MODBUSSETTINGS_H
