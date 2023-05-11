#ifndef MODBUS_SLAVEWORKER_H
#define MODBUS_SLAVEWORKER_H

#include "broker/workers/worker.h"
#include <QModbusDataUnit>
#include <QModbusDevice>

namespace Settings{
struct ModbusSlave;
}
class QModbusServer;
namespace Modbus {

class RADAPTER_API Slave : public Radapter::Worker
{
    Q_OBJECT
    struct Private;
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
    void connectDevice();
    void disconnectDevice();
    void handleNewWords(QVector<quint16> &words, QModbusDataUnit::RegisterType table, int address, int size);

    Private *d;
};

} // namespace Modbus

#endif // MODBUS_SLAVEWORKER_H









