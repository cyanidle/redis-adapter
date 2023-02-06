#ifndef MODBUS_SLAVEWORKER_H
#define MODBUS_SLAVEWORKER_H

#include <QModbusServer>
#include <QTimer>
#include "broker/worker/worker.h"
#include "settings/modbussettings.h"

namespace Modbus {

class RADAPTER_SHARED_SRC Slave : public Radapter::Worker
{
    Q_OBJECT
public:
    Slave(const Settings::ModbusSlaveWorker &settings, QThread *thread);
    ~Slave();
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
    QModbusDataUnit parseValueToDataUnit(const QVariant &src, const Settings::RegisterInfo &regInfo);

    Settings::ModbusSlaveWorker m_settings;
    QTimer *m_reconnectTimer = nullptr;
    QHash<QModbusDataUnit::RegisterType, QHash<int /*index*/, QString>> m_reverseRegisters;
    QModbusServer *modbusDevice = nullptr;
};

} // namespace Modbus

#endif // MODBUS_SLAVEWORKER_H









