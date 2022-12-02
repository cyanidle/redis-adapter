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
    const Formatters::Dict &currentRegisters() const {return m_currRegisters;}
public slots:
    virtual void onMsg(const Radapter::WorkerMsg &msg) override;
private slots:
    void onDataWritten(QModbusDataUnit::RegisterType table, int address, int size);
    void onErrorOccurred(QModbusDevice::Error error);
    void onStateChanged(QModbusDevice::State state);
private:
    Settings::DeviceRegistersInfo &deviceRegisters() {return m_settings.registers;}
    QVariant parseType(QModbusDataUnit::RegisterType table, int address, const Settings::RegisterInfo &reg, int *currentSize, bool *ok);
    static QVariant parseBuffer(quint16 *wordBuffer, const int size, const Settings::PackingMode &packing);
    void connectDevice();
    void disconnectDevice();

    Settings::ModbusSlaveWorker m_settings;
    Formatters::Dict m_currRegisters;
    QTimer *m_reconnectTimer = nullptr;
    QHash<QModbusDataUnit::RegisterType, QHash<int /*index*/, QString>> m_reverseRegisters;
    QModbusServer *modbusDevice = nullptr;
};

} // namespace Modbus

#endif // MODBUS_SLAVEWORKER_H
