#ifndef MODBUSCONNECTOR_H
#define MODBUSCONNECTOR_H

#include <QObject>
#include <QThread>
#include "radapter-broker/singletonbase.h"
#include "redis-adapter/protocol.h"
#include "redis-adapter/settings/settings.h"
#include "lib/modbus/modbusdevicesgate.h"
#include "lib/modbus/modbusfactory.h"
#include "json-formatters/formatters/dict.h"
#include "json-formatters/formatters/list.h"

class RADAPTER_SHARED_SRC ModbusConnector : public Radapter::SingletonBase
{
    Q_OBJECT
public:
    static ModbusConnector* instance();
    static void init(const Settings::ModbusConnectionSettings &connectionSettings,
                     const Settings::DeviceRegistersInfoMap &devices,
                     const Radapter::WorkerSettings &settings);

    Settings::ModbusConnectionSettings settings() const;
    int initSettings() override {return 0;}
    int init() override;
    void run() override;
    Radapter::WorkerMsg::SenderType workerType() const override {return Radapter::WorkerMsg::TypeModbusConnector;}
public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
    void onReply(const Radapter::WorkerMsg &msg) override;
    void onJsonDataReady(const QVariant &nestedUnitData);
private slots:
    void writeJson(const Formatters::Dict &jsonDict, bool *isAbleToWrite);
    void onWriteResultReady(const QVariant &modbusUnitJson, bool successful);
    void onConnectionChanged(const QStringList &deviceNames, const bool connected);
    void emitWriteDone(const Formatters::Dict &writeRequest);
private:
    //Send Commands
    void writeJsonDone(const Formatters::Dict &jsonDict);
    void jsonItemWritten(const Formatters::Dict &modbusJsonUnit);
    void allDevicesConnected();
    void approvalRequested(const Formatters::List &jsonKeys);
    //Receive Reply
    void onApprovalReceived(const Formatters::Dict &jsonDict);


private:
    explicit ModbusConnector(const Settings::ModbusConnectionSettings &connectionSettings,
                             const Settings::DeviceRegistersInfoMap &registersInfo,
                             const Radapter::WorkerSettings &settings);
    ~ModbusConnector() override;
    static ModbusConnector& prvInstance(const Settings::ModbusConnectionSettings &connectionSettings = Settings::ModbusConnectionSettings{},
                                        const Settings::DeviceRegistersInfoMap &devices = Settings::DeviceRegistersInfoMap{},
                                        const Radapter::WorkerSettings &settings = Radapter::WorkerSettings());
    void initServices(const Settings::ModbusConnectionSettings &connectionSettings,
                      const Settings::DeviceRegistersInfoMap &registersInfo);

    QString lastWriteId() const;
    void setLastWriteId(const QString &id);

    ModbusDevicesGate* m_gate;
    Modbus::ModbusFactory* m_modbusFactory;
    QThread* m_modbusThread;
    QString m_lastWriteId;
    Formatters::Dict m_lastWriteRequest;
    Radapter::Protocol* m_proto;
    QList<quint64> m_awaitingApproval;
};

#endif // MODBUSCONNECTOR_H
