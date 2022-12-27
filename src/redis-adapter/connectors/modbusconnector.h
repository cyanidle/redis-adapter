#ifndef MODBUSCONNECTOR_H
#define MODBUSCONNECTOR_H

#include <QObject>
#include <QThread>
#include "radapter-broker/singletonbase.h"
#include "redis-adapter/settings/settings.h"
#include "lib/modbus/modbusdevicesgate.h"
#include "lib/modbus/modbusfactory.h"
#include "jsondict/jsondict.hpp"
#include "jsondict/jsondict.hpp"

class RADAPTER_SHARED_SRC ModbusConnector : public Radapter::SingletonBase
{
    Q_OBJECT
public:
    static ModbusConnector* instance();
    static void init(const Settings::ModbusConnectionSettings &connectionSettings,
                     const Settings::DeviceRegistersInfoMap &devices,
                     const Radapter::WorkerSettings &settings,
                     QThread *thread);

    Settings::ModbusConnectionSettings settings() const;
    int initSettings() override {return 0;}
    int init() override;
    void run() override;
public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
    void onReply(const Radapter::WorkerMsg &msg) override;
    void onJsonDataReady(const QVariant &nestedUnitData);
private slots:
    void writeJson(const JsonDict &jsonDict, bool *isAbleToWrite);
    void onWriteResultReady(const QVariant &modbusUnitJson, bool successful);
    void onConnectionChanged(const QStringList &deviceNames, const bool connected);
    void emitWriteDone(const JsonDict &writeRequest);
private:
    //Send Commands
    void writeJsonDone(const JsonDict &jsonDict);
    void jsonItemWritten(const JsonDict &modbusJsonUnit);
    void allDevicesConnected();
    void approvalRequested(const QStringList &jsonKeys);
    //Receive Reply
    void onApprovalReceived(const JsonDict &jsonDict);


private:
    explicit ModbusConnector(const Settings::ModbusConnectionSettings &connectionSettings,
                             const Settings::DeviceRegistersInfoMap &registersInfo,
                             const Radapter::WorkerSettings &settings,
                             QThread *thread);
    ~ModbusConnector() override;
    static ModbusConnector& prvInstance(const Settings::ModbusConnectionSettings &connectionSettings = Settings::ModbusConnectionSettings{},
                                        const Settings::DeviceRegistersInfoMap &devices = Settings::DeviceRegistersInfoMap{},
                                        const Radapter::WorkerSettings &settings = Radapter::WorkerSettings(),
                                        QThread *thread = nullptr);
    void initServices(const Settings::ModbusConnectionSettings &connectionSettings,
                      const Settings::DeviceRegistersInfoMap &registersInfo);

    QString lastWriteId() const;
    void setLastWriteId(const QString &id);

    ModbusDevicesGate* m_gate;
    Modbus::ModbusFactory* m_modbusFactory;
    QThread* m_modbusThread;
    QString m_lastWriteId;
    JsonDict m_lastWriteRequest;
};

#endif // MODBUSCONNECTOR_H
