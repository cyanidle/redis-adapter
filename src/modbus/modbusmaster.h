#ifndef MODBUS_MASTER_H
#define MODBUS_MASTER_H

#include "broker/worker/worker.h"
#include "settings/modbussettings.h"
#include <QModbusClient>
#include <QQueue>
#include <QObject>

namespace Modbus {

class Master : public Radapter::Worker
{
    Q_OBJECT
public:
    Master(const Settings::ModbusMaster &settings, QThread *thread);
    void onRun() override;
    ~Master() override;
    bool isConnected() const;
signals:
    void requestDone();
    void connected();
    void disconnected();
public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
    void connectDevice();
    void disconnectDevice();
private slots:
    void executeNext();
    void onErrorOccurred(QModbusDevice::Error error);
    void onStateChanged(QModbusDevice::State state);
    void onReadReady();
    void onWriteReady();
    void doRead();
private:
    void enqeueRead(const QModbusDataUnit &unit);
    void enqeueWrite(const QModbusDataUnit &unit);
    void executeRead(const QModbusDataUnit &unit);
    void executeWrite(const QModbusDataUnit &unit);
    const Settings::ModbusMaster &config() const {return m_settings;}

    Settings::ModbusMaster m_settings;
    QTimer *m_reconnectTimer;
    QTimer *m_readTimer;
    QHash<QModbusDataUnit::RegisterType, QHash<int, QString>> m_reverseRegisters{};
    QModbusClient *m_device = nullptr;
    bool m_connected{false};
    QQueue<QModbusDataUnit> m_readQueue{};
    QQueue<QModbusDataUnit> m_writeQueue{};
};

} // namespace Modbus

#endif // MODBUS_MASTER_H
