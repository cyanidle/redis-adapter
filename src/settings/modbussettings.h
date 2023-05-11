#ifndef MODBUSSETTINGS_H
#define MODBUSSETTINGS_H

#include "broker/sync/channel.h"
#include "qthread.h"
#include "settings.h"

Q_DECLARE_METATYPE(QMetaType::Type)

namespace Validator {
    struct ByteWordOrder {
        static const QString &name();
        static bool validate(QVariant &value);
    };
    struct RegValueType {
        static const QString &name();
        static bool validate(QVariant& value);
    };
    struct RegisterTable {
        static const QString &name();
        static bool validate(QVariant& value);
    };
}

namespace Settings {
    using RequiredRegisterTable = ::Serializable::Validated<Required<QModbusDataUnit::RegisterType>>::With<Validator::RegisterTable>;
    using RegisterValueType = ::Serializable::Validated<Required<QMetaType::Type>>::With<Validator::RegValueType>;

    struct RADAPTER_API ModbusQuery : Serializable {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(RequiredRegisterTable, type)
        FIELD(Required<quint16>, reg_index)
        FIELD(Required<quint8>, reg_count)
    };
    struct RADAPTER_API PackingMode : Serializable {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(NonRequiredByteOrder, words, QDataStream::LittleEndian)
        FIELD(NonRequiredByteOrder, bytes, QDataStream::LittleEndian)

        PackingMode() = default;
        PackingMode(QDataStream::ByteOrder words, QDataStream::ByteOrder bytes);
    };

    struct RADAPTER_API ModbusDevice : Serializable {
        typedef QMap<QString, ModbusDevice> Map;
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(Optional<TcpDevice>, tcp)
        FIELD(Optional<SerialDevice>, rtu)

        QSharedPointer<Radapter::Sync::Channel> channel;
        void postUpdate() override;
    };

    struct RADAPTER_API RegisterInfo : Serializable {
        Q_GADGET
        IS_SERIALIZABLE
        using Orders = ::Serializable::Validated<HasDefault<PackingMode>>::With<Validator::ByteWordOrder>;
        FIELD(Orders, endianess)
        FIELD(RequiredRegisterTable, table)
        FIELD(Required<int>, index)
        FIELD(MarkHasDefault<RegisterValueType>, type, QMetaType::UShort)
        FIELD(HasDefault<bool>, resetting, false) // not implemented yet
        FIELD(HasDefault<bool>, writable, true)
        FIELD(OptionalValidator, validator)
        void postUpdate() override;
    };
    struct RegisterCounts {
        quint16 coils;
        quint16 di;
        quint16 holding_registers;
        quint16 input_registers;
        void reset();
    };

    //! Common Modbus worker params
    struct RADAPTER_API ModbusWorker : Serializable {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(Required<Worker>, worker)
        FIELD(Required<QString>, device_name)
        FIELD(Required<quint16>, slave_id)
        FIELD(RequiredSequence<QString>, register_names)
        FIELD(HasDefault<quint32>, reconnect_timeout_ms, 3000)
        FIELD(HasDefault<bool>, read_only, false)
    };

    struct RADAPTER_API ModbusSlave : ModbusWorker {
        Q_GADGET
        IS_SERIALIZABLE

        ModbusDevice device{};
        RegisterCounts counts{};
        QStringMap<RegisterInfo> registers{};

        void postUpdate() override;
    };

    struct RADAPTER_API ModbusMaster : ModbusWorker {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(OptionalSequence<ModbusQuery>, queries)

        FIELD(HasDefault<quint32>, poll_rate, 500)
        FIELD(HasDefault<quint32>, response_time_ms, 150)
        FIELD(HasDefault<quint32>, retries, 3)

        FIELD(Optional<QString>, state_writer)
        FIELD(Optional<QString>, state_reader)

        ModbusDevice device{};
        QStringMap<RegisterInfo> registers{};
        void postUpdate() override;
    };

    struct RADAPTER_API Registers : Serializable {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(OptionalMapping<QVariantMap>, holding)
        FIELD(OptionalMapping<QVariantMap>, holding_registers)
        FIELD(OptionalMapping<QVariantMap>, input)
        FIELD(OptionalMapping<QVariantMap>, input_registers)
        FIELD(OptionalMapping<QVariantMap>, coils)
        FIELD(OptionalMapping<QVariantMap>, discrete_inputs)
        FIELD(OptionalMapping<QVariantMap>, di)
        void postUpdate() override;

        COMMENT(holding, "{index: int, type: (float32/uint32/uint16), validator: (name), endianess: (abcd), writable: (bool)}")
        COMMENT(holding_registers, "{index: int, type: (float32/uint32/uint16), validator: (name), endianess: (abcd), writable: (bool)}")
        COMMENT(input, "{index: int, type: (float32/uint32/uint16), validator: (name), endianess: (abcd), writable: (bool)}")
        COMMENT(input_registers, "{index: int, type: (float32/uint32/uint16), validator: (name), endianess: (abcd), writable: (bool)}")
        COMMENT(coils, "{index: int, type: (float32/uint32/uint16), validator: (name), endianess: (abcd), writable: (bool)}")
        COMMENT(discrete_inputs, "{index: int, type: (float32/uint32/uint16), validator: (name), endianess: (abcd), writable: (bool)}")
        COMMENT(di, "{index: int, type: (float32/uint32/uint16), validator: (name), endianess: (abcd), writable: (bool)}")
    };

}

#endif // MODBUSSETTINGS_H
