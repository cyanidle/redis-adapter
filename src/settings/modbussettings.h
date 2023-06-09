#ifndef MODBUSSETTINGS_H
#define MODBUSSETTINGS_H

#include "settings.h"
#include <QModbusDataUnit>

Q_DECLARE_METATYPE(QMetaType::Type)
namespace Radapter{namespace Sync{class Channel;}}
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
    struct RADAPTER_API PackingMode : Serializable {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(NonRequiredByteOrder, words, QDataStream::LittleEndian)
        FIELD(NonRequiredByteOrder, bytes, QDataStream::LittleEndian)

        PackingMode() = default;
        PackingMode(QDataStream::ByteOrder words, QDataStream::ByteOrder bytes);
    };

    struct RADAPTER_API ModbusDevice : Serializable {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(Optional<TcpDevice>, tcp)
        FIELD(Optional<SerialDevice>, rtu)
        FIELD(HasDefault<quint32>, frame_gap, 25)
        COMMENT(frame_gap, "Interframe gap is the minimal timeout between queries")

        QSharedPointer<Radapter::Sync::Channel> channel;
        void postUpdate() override;
    };

    struct RADAPTER_API RegisterInfo : Serializable {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(VALIDATED(Required<QModbusDataUnit::RegisterType>, Validator::RegisterTable), table)
        FIELD(VALIDATED(HasDefault<PackingMode>, Validator::ByteWordOrder), endianess)
        FIELD(VALIDATED(HasDefault<QMetaType::Type>, Validator::RegValueType), type, QMetaType::UShort)
        FIELD(Required<int>, index)
        FIELD(HasDefault<bool>, resetting, false) // not implemented yet
        FIELD(HasDefault<bool>, writable, true)
        FIELD(HasDefault<bool>, readable, true)
        FIELD(Optional<QString>, mode)
        COMMENT(mode, "Allowed: r / w / rw (read, write, read+write)")
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
        FIELD(Required<QString>, device)
        FIELD(Required<quint16>, slave_id)
        FIELD(Required<QString>, registers)
        FIELD(HasDefault<quint32>, reconnect_timeout_ms, 1000)

        ModbusDevice m_device;
        QStringMap<RegisterInfo> m_registers;
        void init();
    };

    struct RADAPTER_API ModbusSlave : ModbusWorker {
        Q_GADGET
        IS_SERIALIZABLE
        RegisterCounts counts{};
        void init();
    };

    struct RADAPTER_API ModbusMaster : ModbusWorker {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(HasDefault<quint32>, poll_rate, 500)
        COMMENT(poll_rate, "Poll rate is the time between two full updates")
        FIELD(HasDefault<quint32>, response_time, 150)
        FIELD(HasDefault<quint32>, retries, 3)

        FIELD(Optional<QString>, state_writer)
        FIELD(Optional<QString>, state_reader)
    };

    struct RADAPTER_API Registers : Serializable {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(Optional<QVariantMap>, holding)
        FIELD(Optional<QVariantMap>, holding_registers)
        FIELD(Optional<QVariantMap>, input)
        FIELD(Optional<QVariantMap>, input_registers)
        FIELD(Optional<QVariantMap>, coils)
        FIELD(Optional<QVariantMap>, discrete_inputs)
        FIELD(Optional<QVariantMap>, di)

        FIELD(HasDefault<bool>, allow_read_by_default, true)
        FIELD(HasDefault<bool>, allow_write_by_default, true)
        void init(const QString &device);
    };

}

#endif // MODBUSSETTINGS_H
