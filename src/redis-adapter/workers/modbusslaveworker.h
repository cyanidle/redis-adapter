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
    static QVariant parseType(quint16 *words, const Settings::RegisterInfo &regInfo, int sizeWords);
    void connectDevice();
    void disconnectDevice();
    QModbusDataUnit parseValues(const QVariant &src, const Settings::RegisterInfo &regInfo);

    Settings::ModbusSlaveWorker m_settings;
    QTimer *m_reconnectTimer = nullptr;
    QHash<QModbusDataUnit::RegisterType, QHash<int /*index*/, QString>> m_reverseRegisters;
    QModbusServer *modbusDevice = nullptr;
};

template<typename T>
typename std::enable_if<!(std::is_pointer<T>())>::type
flipWords(T& target) {
    static_assert(!(sizeof(T)%2), "Odd words count types unsupported!");
    constexpr const auto sizeWords = sizeof(T)/2;
    auto words = reinterpret_cast<quint16*>(&target);
    for (size_t i = 0; i < sizeWords / 2; ++i) {
        std::swap(words[i], words[sizeWords - 1 - i]);
    }
}

inline void flipBytes(quint16 *words, int sizeWords) {
    auto bytes = reinterpret_cast<quint8*>(words);
    for (int i = 0; i < sizeWords; ++i) {
        std::swap(bytes[i], bytes[sizeWords - 1 - i]);
    }
}

inline void applyEndianness(quint16 *words, const Settings::PackingMode endianess, int sizeWords) {
    if (endianess.byte_order == QDataStream::LittleEndian) {
        flipBytes(words, sizeWords);
    }
    if (endianess.word_order == QDataStream::BigEndian) {
        flipWords(*words);
    }
}

} // namespace Modbus

#endif // MODBUS_SLAVEWORKER_H









