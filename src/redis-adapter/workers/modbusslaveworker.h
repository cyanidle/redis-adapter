#ifndef MODBUS_SLAVEWORKER_H
#define MODBUS_SLAVEWORKER_H

#include <QModbusServer>
#include <QTimer>
#include "radapter-broker/workerbase.h"
#include "redis-adapter/settings/modbussettings.h"
#include "redis-adapter/formatters/wordoperations.h"

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
    QModbusDataUnit setValues(const QVariant &src, const Settings::RegisterInfo &regInfo);

    Settings::ModbusSlaveWorker m_settings;
    QTimer *m_reconnectTimer = nullptr;
    QHash<QModbusDataUnit::RegisterType, QHash<int /*index*/, QString>> m_reverseRegisters;
    QModbusServer *modbusDevice = nullptr;
};

} // namespace Modbus

#endif // MODBUS_SLAVEWORKER_H









