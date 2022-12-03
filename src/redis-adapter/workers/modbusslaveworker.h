#ifndef MODBUS_SLAVEWORKER_H
#define MODBUS_SLAVEWORKER_H

#include <QModbusServer>
#include <QTimer>
#include "radapter-broker/workerbase.h"
#include "redis-adapter/settings/modbussettings.h"

namespace Modbus {

class RADAPTER_SHARED_SRC SlaveWorker : public Radapter::WorkerBase
{
    Q_OBJECT
public:
    SlaveWorker(const Settings::ModbusSlaveWorker &settings, QThread *thread);
    void run() override;
    ~SlaveWorker();
public slots:
    virtual void onMsg(const Radapter::WorkerMsg &msg) override;
private slots:
    void onDataWritten(QModbusDataUnit::RegisterType table, int address, int size);
    void onErrorOccurred(QModbusDevice::Error error);
    void onStateChanged(QModbusDevice::State state);
private:
    Settings::DeviceRegistersInfo &deviceRegisters() {return m_settings.registers;}
    inline QVariant parseType(QModbusDataUnit::RegisterType table, int address, const Settings::RegisterInfo &reg, int *currentSize);
    static inline void applyEndianess(quint16 *wordBuffer, const int sizeWords, const Settings::PackingMode &endianess);
    void connectDevice();
    void disconnectDevice();
    void setValues(const QVariant &src, const Settings::RegisterInfo &regInfo);

    Settings::ModbusSlaveWorker m_settings;
    QTimer *m_reconnectTimer = nullptr;
    QHash<QModbusDataUnit::RegisterType, QHash<int /*index*/, QString>> m_reverseRegisters;
    QModbusServer *modbusDevice = nullptr;
};

void SlaveWorker::applyEndianess(quint16 *wordBuffer, const int sizeWords, const Settings::PackingMode &endianess) {
    if (endianess.word_order == QDataStream::ByteOrder::BigEndian) {
        for (int i = 0; i < sizeWords / 2; ++i) {
            std::swap(*(wordBuffer + i), *(wordBuffer + sizeWords - i - 1));
        }
    }
    if (endianess.byte_order == QDataStream::ByteOrder::LittleEndian) {
        for (int i = 0; i < sizeWords; ++i) {
            auto bytesInWord = reinterpret_cast<quint8*>(wordBuffer + i);
            std::swap(*bytesInWord, *(bytesInWord + 1));
        }
    }
}

QVariant SlaveWorker::parseType(QModbusDataUnit::RegisterType table, int address,
                                const Settings::RegisterInfo &reg, int *currentSize)
{
    constexpr static int maxSize = sizeof(long long int);
    const int sizeWords = QMetaType::sizeOf(reg.type)/2 + QMetaType::sizeOf(reg.type) % 2;
    if (sizeWords > maxSize || sizeWords <= 0) {
        return {};
    }
    *currentSize += sizeWords - 1;
    quint16 buffer[maxSize] = {};
    for (int i = 0; i < sizeWords; ++i) {
        bool ok = false;
        switch (table) {
        case QModbusDataUnit::Coils:
            ok = modbusDevice->data(QModbusDataUnit::Coils, quint16(address + i), buffer + i);
            break;
        case QModbusDataUnit::DiscreteInputs:
            ok = modbusDevice->data(QModbusDataUnit::DiscreteInputs, quint16(address + i), buffer + i);
            break;
        case QModbusDataUnit::HoldingRegisters:
            ok = modbusDevice->data(QModbusDataUnit::HoldingRegisters, quint16(address + i), buffer + i);
            break;
        case QModbusDataUnit::InputRegisters:
            ok = modbusDevice->data(QModbusDataUnit::InputRegisters, quint16(address + i), buffer + i);
            break;
        default:
            reWarn() << "Invalid data written: Adress: " << address + i << "; Table: " << table;
            return {};
        }
        if (!ok) {
            reWarn() << "Error reading data! Adress:" << address << "; Table:" << table;
        }
    }
    applyEndianess(buffer, QMetaType::sizeOf(reg.type)/2, reg.endianess);
    switch (reg.type) {
    case QMetaType::Float:
        return *reinterpret_cast<float*>(buffer);
    case QMetaType::UShort:
        return *reinterpret_cast<quint16*>(buffer);
    case QMetaType::UInt:
        return *reinterpret_cast<quint32*>(buffer);
    case QMetaType::UChar:
        return *(reinterpret_cast<quint8*>(buffer) +
                 (reg.endianess.byte_order == QDataStream::BigEndian));
    default:
        return {};
    }
}

} // namespace Modbus

#endif // MODBUS_SLAVEWORKER_H









