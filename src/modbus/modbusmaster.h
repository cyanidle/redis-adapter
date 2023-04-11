#ifndef MODBUS_MASTER_H
#define MODBUS_MASTER_H

#include "broker/workers/worker.h"
#include "settings/modbussettings.h"
#include "modbusparsing.h"
#include <QModbusReply>
#include <QModbusDevice>
#include <QQueue>
#include <QObject>

class QTimer;
class QModbusClient;
namespace Redis {
class CacheProducer;
class CacheConsumer;
}
namespace Modbus {

class Master : public Radapter::Worker
{
    Q_OBJECT
public:
    Master(const Settings::ModbusMaster &settings, QThread *thread);
    void onRun() override;
    ~Master() override;
    bool isConnected() const;
    const Settings::ModbusMaster &config() const;
signals:
    void askTrigger();
    void queryDone();
    void allQueriesDone();
    void connected();
    void disconnected();
public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
    void connectDevice();
private slots:
    void onReadReady();
    void onWriteReady();
    void doRead();
    void onErrorOccurred(QModbusDevice::Error error);
    void onStateChanged(QModbusDevice::State state);
    void reconnect();
private:
    void formatAndSendJson(const JsonDict &json);
    void enqeueRead(const QModbusDataUnit &unit);
    void enqeueWrite(const QModbusDataUnit &unit);
    void executeNext();
    void initClient();
    void executeRead(const QModbusDataUnit &unit);
    void executeWrite(const QModbusDataUnit &state);
    void saveState();
    void fetchState();
    void write(const JsonDict &data);
    void attachToChannel();

    Settings::ModbusMaster m_settings;
    QTimer *m_reconnectTimer;
    QTimer *m_readTimer;
    QHash<QModbusDataUnit::RegisterType, QHash<int, QString>> m_reverseRegisters;
    QModbusClient *m_device = nullptr;
    std::atomic<bool> m_connected{false};
    QQueue<QModbusDataUnit> m_readQueue;
    QQueue<QModbusDataUnit> m_writeQueue;
    JsonDict m_state;
    JsonDict m_wantedState;
    QMap<QString, quint8> m_rewriteAttempts;
    Redis::CacheProducer *m_stateWriter;
    Redis::CacheConsumer *m_stateReader;
};

} // namespace Modbus

#endif // MODBUS_MASTER_H
