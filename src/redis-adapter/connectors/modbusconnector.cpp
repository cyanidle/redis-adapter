#include "modbusconnector.h"
#include "redis-adapter/formatters/modbusformatter.h"
#include "redis-adapter/formatters/streamentriesmapformatter.h"
#include "redis-adapter/include/modbuskeys.h"
#include "radapter-broker/broker.h"
#include "redis-adapter/radapterlogging.h"
#include "redis-adapter/radapterschemas.h"

using namespace Radapter;

ModbusConnector::ModbusConnector(const Settings::ModbusConnectionSettings &connectionSettings,
                                 const Settings::DeviceRegistersInfoMap &registersInfo,
                                 const Radapter::WorkerSettings &settings)
    : SingletonBase(settings),
      m_lastWriteId{},
      m_lastWriteRequest{}
{
    initServices(connectionSettings, registersInfo);
}

ModbusConnector::~ModbusConnector()
{
    m_modbusThread->quit();
}

ModbusConnector *ModbusConnector::instance()
{
    return &prvInstance();
}

int ModbusConnector::init()
{
    return 0;
}

void ModbusConnector::writeJsonDone(const Formatters::Dict &jsonDict)
{
    emit sendMsgWithDirection(prepareCommand<Radapter::AcknowledgeSchema>(jsonDict), DirectionToProducers);
}

void ModbusConnector::jsonItemWritten(const Formatters::Dict &modbusJsonUnit)
{
    Q_UNUSED(modbusJsonUnit);
    emit sendMsgWithDirection(prepareCommand<Radapter::RequestJsonSchema>(true), DirectionToProducers);
}

void ModbusConnector::allDevicesConnected()
{
    emit sendMsgWithDirection(prepareCommand<Radapter::RequestJsonSchema>(true), DirectionToProducers);
}

ModbusConnector &ModbusConnector::prvInstance(const Settings::ModbusConnectionSettings &connectionSettings,
                                              const Settings::DeviceRegistersInfoMap &devices,
                                              const Radapter::WorkerSettings &settings)
{
    static ModbusConnector conn(connectionSettings, devices, settings);
    return conn;
}

void ModbusConnector::init(const Settings::ModbusConnectionSettings &connectionSettings,
                           const Settings::DeviceRegistersInfoMap &devices,
                           const Radapter::WorkerSettings &settings)
{
    prvInstance(connectionSettings, devices, settings);
}

void ModbusConnector::initServices(const Settings::ModbusConnectionSettings &connectionSettings, const Settings::DeviceRegistersInfoMap &registersInfo)
{
    m_modbusThread = new QThread();
    m_modbusFactory = new Modbus::ModbusFactory();
    m_modbusFactory->createDevices(connectionSettings);
    auto devicesInfo = m_modbusFactory->devicesInfo();
    m_modbusFactory->moveToThread(m_modbusThread);
    connect(m_modbusThread, &QThread::started, m_modbusFactory, &Modbus::ModbusFactory::run);
    connect(m_modbusThread, &QThread::finished, m_modbusFactory, &Modbus::ModbusFactory::deleteLater);
    connect(m_modbusThread, &QThread::finished, m_modbusThread, &QThread::deleteLater);

    ModbusDevicesGate::init(registersInfo, devicesInfo, nullptr);
    m_gate = ModbusDevicesGate::instance();

    connect(m_modbusFactory, &Modbus::ModbusFactory::dataChanged,
            m_gate, &ModbusDevicesGate::readModbusData,
            Qt::ConnectionType::QueuedConnection);
    connect(m_gate, &ModbusDevicesGate::deviceWriteRequested,
            m_modbusFactory, &Modbus::ModbusFactory::changeDeviceData,
            Qt::ConnectionType::QueuedConnection);
    connect(m_modbusFactory, &Modbus::ModbusFactory::writeResultReady,
            m_gate, &ModbusDevicesGate::receiveDeviceWriteResult,
            Qt::ConnectionType::QueuedConnection);
    connect(m_modbusFactory, &Modbus::ModbusFactory::initRequested,
            m_gate, &ModbusDevicesGate::initModbusDevice,
            Qt::ConnectionType::QueuedConnection);

    connect(m_gate, &ModbusDevicesGate::jsonDataReady, this, &ModbusConnector::onJsonDataReady, Qt::QueuedConnection);
    connect(m_gate, &ModbusDevicesGate::deviceWriteResultReady, this, &ModbusConnector::onWriteResultReady, Qt::QueuedConnection);
    connect(m_modbusFactory, &Modbus::ModbusFactory::connectionChanged,
            this, &ModbusConnector::onConnectionChanged,
            Qt::QueuedConnection);
}

QString ModbusConnector::lastWriteId() const
{
    return m_lastWriteId;
}

void ModbusConnector::setLastWriteId(const QString &id)
{
    if (m_lastWriteId != id) {
        m_lastWriteId = id;
    }
}

Settings::ModbusConnectionSettings ModbusConnector::settings() const
{
    return m_modbusFactory->connectionSettings();
}

void ModbusConnector::run()
{
    m_modbusThread->start();
    thread()->start();
}

void ModbusConnector::onMsg(const Radapter::WorkerMsg &msg)
{
    auto jsonDict = msg.data();
    if (jsonDict.isEmpty()) {
        return;
    }

    if (!StreamEntriesMapFormatter::isValid(jsonDict)) {
        return;
    }
    auto jsonUnit = StreamEntriesMapFormatter(jsonDict).joinToLatest();
    auto lastMessageId = jsonDict.lastKey();
    if (lastMessageId == lastWriteId()) {
        auto jsonKeys = Formatters::List{ jsonUnit.keys() };
        m_lastWriteRequest = jsonDict;
        approvalRequested(jsonKeys);
    } else {
        bool isAbleToWrite = false;
        writeJson(jsonUnit, &isAbleToWrite);
        if (!isAbleToWrite) {
            // dead letter received
            emitWriteDone(jsonDict);
        }
    }
    setLastWriteId(lastMessageId);
}

void ModbusConnector::approvalRequested(const Formatters::List &jsonKeys)
{
    auto command = prepareCommand<RequestKeysSchema>(jsonKeys);
    emit sendMsgWithDirection(command, DirectionToProducers);
}

void ModbusConnector::onReply(const Radapter::WorkerMsg &msg)
{
    if (msg.usesSchema<AcknowledgeSchema>()) {
        onApprovalReceived(msg.schemaAs<AcknowledgeSchema>()->receiveAckJson(msg).data());
    } else {
        reDebug() << "ModbusConnector received reply with non 'AcknowledgeSchema'";
    }
}

void ModbusConnector::onApprovalReceived(const Formatters::Dict &jsonDict)
{
    if (jsonDict.isEmpty()) {
        return;
    }
    auto lastRequest = StreamEntriesMapFormatter(m_lastWriteRequest).joinToLatest();
    if (lastRequest.containsDict(jsonDict)) {
        emitWriteDone(m_lastWriteRequest);
    }
}

void ModbusConnector::writeJson(const Formatters::Dict &jsonUnit, bool *isAbleToWrite)
{
    if (jsonUnit.isEmpty()) {
        return;
    }

    auto modbusUnit = ModbusFormatter(jsonUnit).toModbusUnit();
    if (!modbusUnit.isEmpty()) {
        m_gate->writeJsonToModbusDevice(modbusUnit, isAbleToWrite);
    }
}

void ModbusConnector::onJsonDataReady(const QVariant &nestedUnitData)
{
    auto json = prepareMsg(Formatters::Dict(nestedUnitData));
    if (!json.isEmpty()) {
        emit sendMsg(json);
    }
}

void ModbusConnector::onWriteResultReady(const QVariant &modbusUnitData, bool successful)
{
    if (successful) {
        auto modbusJsonUnit = Formatters::Dict(modbusUnitData);
        jsonItemWritten(modbusJsonUnit);
    }
}

void ModbusConnector::onConnectionChanged(const QStringList&, const bool connected)
{
    if (connected && m_modbusFactory->areAllDevicesConnected()) {
        allDevicesConnected();
    }
}

void ModbusConnector::emitWriteDone(const Formatters::Dict &writeRequest)
{
    writeJsonDone(writeRequest);
    m_lastWriteRequest = Formatters::Dict{};
}
