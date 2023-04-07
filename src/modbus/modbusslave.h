#ifndef MODBUS_SLAVEWORKER_H
#define MODBUS_SLAVEWORKER_H

#include <QModbusDevice>
#include <QTimer>
#include "broker/workers/worker.h"
#include "settings/modbussettings.h"

class QModbusServer;
namespace Modbus {

class RADAPTER_API Slave : public Radapter::Worker
{
    Q_OBJECT
public:
    Slave(const Settings::ModbusSlave &settings, QThread *thread);
    ~Slave();
    bool isConnected() const;
public slots:
    virtual void onMsg(const Radapter::WorkerMsg &msg) override;
private slots:
    void onDataWritten(QModbusDataUnit::RegisterType table, int address, int size);
    void onErrorOccurred(QModbusDevice::Error error);
    void onStateChanged(QModbusDevice::State state);
private:
    Settings::Registers &deviceRegisters() {return m_settings.registers;}
    void connectDevice();
    void disconnectDevice();
    const Settings::ModbusSlave &config() const {return m_settings;}
    void handleNewWords(QVector<quint16> &words, QModbusDataUnit::RegisterType table, int address, int size);

    Settings::ModbusSlave m_settings;
    QTimer *m_reconnectTimer = nullptr;
    QHash<QModbusDataUnit::RegisterType, QHash<int /*index*/, QString>> m_reverseRegisters;
    QModbusServer *modbusDevice = nullptr;
    std::atomic<bool> m_connected{false};
    JsonDict m_state{};
};

} // namespace Modbus

#endif // MODBUS_SLAVEWORKER_H









