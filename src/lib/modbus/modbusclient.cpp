#include "modbusclient.h"
#include <QtSerialBus/QModbusRtuSerialMaster>
#include <QtSerialBus/QModbusTcpClient>
#include <QRandomGenerator>
#include <QSerialPortInfo>
#include "redis-adapter/radapterlogging.h"

using namespace Modbus;
using namespace Settings;

#define PREALLOC_REG_SIZE   3072U
#define WRITE_REQUEST_SIZE  1U
#ifdef TEST_MODE
    #define TEST_REQUEST_SIZE 5U
#endif

#define RECONNECT_TIME_MS           5000
#define SEARCH_START_COUNTER        3
#define MAX_ERROR_COUNT             5u

ModbusClient::ModbusClient(const ModbusConnectionSource &settings,
                           const ModbusQueryMap &queryMap,
                           const QStringList &deviceNames,
                           QObject *parent)
    : QObject(parent),
      m_client(nullptr),
      m_queryDataMap(queryMap),
      m_readQueries{},
      m_writeQueries{},
      m_deviceBuffersMap{},
      m_connectionSettings(settings),
      m_deviceNames(deviceNames),
      m_deviceInfoList{},
      m_remappedPort{},
      m_reconnectCounter(0u),
      m_clientConnected(false),
      m_areAllSlavesDisconnected(false)
{
    m_connectionType = settings.type;
    init();
    connect(this, &ModbusClient::writeRequested,
            this, &ModbusClient::sendWriteRequest);
    connect(this, &ModbusClient::allSlavesDisconnected,
            this, &ModbusClient::restartDevice);
    connect(this, &ModbusClient::slaveStateChanged,
            this, &ModbusClient::onSlaveStateChanged);
    const auto addressesList = queryMap.uniqueKeys();
    for (auto slaveAddress : addressesList) {
        m_deviceBuffersMap[slaveAddress] = {            
            { QModbusDataUnit::DiscreteInputs, RegistersTableBuffer(PREALLOC_REG_SIZE, 0u) },
            { QModbusDataUnit::Coils, RegistersTableBuffer(PREALLOC_REG_SIZE, 0u) },
            { QModbusDataUnit::InputRegisters, RegistersTableBuffer(PREALLOC_REG_SIZE, 0u) },
            { QModbusDataUnit::HoldingRegisters, RegistersTableBuffer(PREALLOC_REG_SIZE, 0u) }

        };
    }
}

ModbusClient::~ModbusClient()
{
    m_scheduler->deleteLater();
    deviceDisconnect();
}

void ModbusClient::init()
{
    auto initialDeviceState = ModbusConnectionState{};
    initialDeviceState.isConnected = true;
    for (auto queryData = m_queryDataMap.begin();
         queryData != m_queryDataMap.end();
         queryData++)
    {
        auto slaveAddress = queryData.key();
        m_connectionStatesMap[slaveAddress] = initialDeviceState;
        if (hasDeviceInfo(slaveAddress)) {
            continue;
        }
        auto deviceInfo = createDeviceInfo(slaveAddress);
        if (deviceInfo.isValid()) {
            m_deviceInfoList.append(deviceInfo);
        }
    }
}

void ModbusClient::deviceConnect()
{
    if (m_client == nullptr) {
        mbDebug() << QString("Client %1 is NULL. Creating device...").arg(channelId());
        m_client = createDevice();

        setConnectionParameters();
        m_client->disconnect();
        connect(m_client, &QModbusClient::stateChanged,
                this, &ModbusClient::deviceStateChanged);
    }
    if (m_client) {
        tryConnect();
    }
}

void ModbusClient::deviceDisconnect()
{
    mbDebug() << "start device disconnect";
    if (!m_readQueries.isEmpty()) {
        for (auto query : qAsConst(m_readQueries)) {
            destroyQuery(query);
        }
        m_readQueries.clear();
    }
    mbDebug() << "read queries deleted";
    if (!m_writeQueries.isEmpty()) {
        for (auto query : qAsConst(m_writeQueries)) {
            destroyQuery(query);
        }
        m_writeQueries.clear();
    }
    mbDebug() << "write queries deleted";

    if (m_client) {
        disconnect(m_client, &QModbusClient::stateChanged,
                   this, &ModbusClient::deviceStateChanged);
        connect(m_client, &QModbusClient::stateChanged, this, [=]() {
            mbDebug() << "requesting client deleteLater...";
            m_client->deleteLater();
            mbDebug() << "client deleteLater request complete. It is NULL.";
            m_client = nullptr;
        });
        mbDebug() << "disconnecting client...";
        m_client->disconnectDevice();
    }
}

bool ModbusClient::isConnected() const
{
    return m_clientConnected;
}

void ModbusClient::setConnected(bool state)
{
    if (m_clientConnected != state) {
        m_clientConnected = state;
        emit connectionChanged(state);
    }
}

void ModbusClient::run()
{
    m_scheduler = new ModbusScheduler{};
    connect(this, &ModbusClient::connectionChanged,
            this, &ModbusClient::startPoll);
    deviceConnect();
}

QString ModbusClient::channelId() const
{
    return m_deviceInfoList.first().connectionString();
}

QString ModbusClient::currentSerialPort() const
{
    if (!m_client) {
        return QString{};
    }
    return m_client->connectionParameter(QModbusDevice::SerialPortNameParameter).toString();
}

QString ModbusClient::description() const
{
    auto description = channelId();
    if (m_connectionType == Serial) {
        description += " @ " + currentSerialPort();
    }
    return description;
}

ModbusConnectionType ModbusClient::type() const
{
    return m_connectionType;
}

QString ModbusClient::sourceName() const
{
    return m_connectionSettings.name;
}

ModbusDeviceInfoList ModbusClient::devicesInfo() const
{
    return m_deviceInfoList;
}

QVector<quint8> ModbusClient::devicesIdList() const
{
    auto devicesIdList = QVector<quint8>{};
    for (auto &deviceInfo : devicesInfo()) {
        if (!devicesIdList.contains(deviceInfo.slaveAddress())) {
            devicesIdList.append(deviceInfo.slaveAddress());
        }
    }
    return devicesIdList;
}

void ModbusClient::writeReg(const quint8 slaveAddress, const QModbusDataUnit::RegisterType tableType,
                            const quint16 address, const quint16 value)
{

    if (m_deviceBuffersMap.value(slaveAddress).value(tableType).at(address) == value) {
        emit writeResultReady(m_deviceNames, slaveAddress, tableType, address, true);
    } else {
        auto request = createRequest(tableType, address, value);
        mbDebug() << QString("single request for %1 sended:").arg(slaveAddress).toStdString().c_str()
                  << registerTypeToString(request.registerType()).c_str()
                  << request.startAddress() << request.values();
        emit writeRequested(slaveAddress, request);
    }
}

void ModbusClient::writeRegs(const quint8 slaveAddress, const QModbusDataUnit::RegisterType tableType,
     const quint16 startAddress, const QVector<quint16> &values)
{
    auto writeRequests = QVector<QModbusDataUnit>{};

    for (quint16 index = 0; index < values.count(); index++) {
        quint16 address = startAddress + index;
        auto regValue = values.at(index);
        if (m_deviceBuffersMap.value(slaveAddress)
                .value(tableType).at(address) != regValue)
        {
            auto request = createRequest(tableType, address, regValue);
            writeRequests.append(request);
        }
    }

    if (!writeRequests.isEmpty()) {
        for (auto &request : writeRequests) {
            mbDebug() << QString("request for %1 sended:").arg(slaveAddress).toStdString().c_str()
                      << registerTypeToString(request.registerType()).c_str()
                      << request.startAddress() << request.values();
            emit writeRequested(slaveAddress, request);
        }
    }
}

void ModbusClient::changeData(const quint8 slaveAddress,
                              const QModbusDataUnit::RegisterType tableType,
                              const ModbusRegistersTable &registersTable)
{
    for (auto regData = registersTable.begin();
         regData != registersTable.end();
         regData++)
    {
        writeReg(slaveAddress, tableType, regData.key(), regData.value());
    }
}

void ModbusClient::restartDevice()
{
    deviceDisconnect();
    deviceConnect();
}

void ModbusClient::startPoll()
{
    if (!isConnected())
        return;

    if (!m_readQueries.isEmpty() && !m_writeQueries.isEmpty()) {
        //startDelayedQueries();
        return;
    }

    for (auto queryData = m_queryDataMap.begin();
         queryData != m_queryDataMap.end();
         queryData++)
    {
        auto query = createReadQuery(queryData.key(), queryData.value().dataUnit, queryData.value().pollRate);
        m_scheduler->append(query);
    }
    m_firstReadDone = false;
    m_areAllSlavesDisconnected = false;
    resetAllConnections();
    m_scheduler->start();
}

void ModbusClient::stopSlavePoll(const quint8 slaveAddress)
{
    const auto slaveQueries = getReadQueriesBySlaveAddress(slaveAddress);
    for (auto query : slaveQueries) {
        m_scheduler->remove(query);
        m_readQueries.removeAll(query);
        destroyQuery(query);
    }
}

void ModbusClient::deviceStateChanged(const QModbusDevice::State state)
{
    bool connected = m_client && state == QModbusDevice::ConnectedState;
    if (m_client && (state == QModbusDevice::UnconnectedState
                     || state == QModbusDevice::ClosingState ))
    {
        m_reconnectCounter++;
        mbDebug() << QString("Cannot connect to Modbus device %1 at %2 time!")
                     .arg(description()).arg(m_reconnectCounter);

        if (m_connectionType == Serial) {
            updateToNewPort();
        }

        QTimer::singleShot(RECONNECT_TIME_MS, this, &ModbusClient::deviceConnect);
    }

    setConnected(connected);
}

void ModbusClient::readReady(const quint8 slaveAddress, const QModbusDataUnit reply)
{
    if (!reply.isValid()) {
        return;
    }

    auto updatedRegs = updateRegisters(m_deviceBuffersMap, slaveAddress, reply);
    if (!updatedRegs.isEmpty()) {
        emit dataChanged(slaveAddress, updatedRegs);
    }
    outputDataUnit(slaveAddress, reply);
}

void ModbusClient::queryFailed(const quint8 slaveAddress, const QString error)
{
    mbDebug() << Q_FUNC_INFO << "slave" << slaveAddress << error;
    // error handling
}

void ModbusClient::countErrors(const quint8 slaveAddress)
{
    if (m_areAllSlavesDisconnected) {
        return;
    }

    ModbusConnectionState& deviceState = m_connectionStatesMap[slaveAddress];
    if (!deviceState.isConnected) {
        return;
    }

    deviceState.errorsCounter++;
    mbDebug() << Q_FUNC_INFO << this << slaveAddress << "errorsCounter:" << deviceState.errorsCounter;
    if (deviceState.errorsCounter > MAX_ERROR_COUNT) {
        deviceState.errorsCounter = MAX_ERROR_COUNT;
        mbDebug() << Q_FUNC_INFO << this << "slave" << slaveAddress << "has disconnected";
        setDeviceConnected(slaveAddress, &deviceState, false);
    }
    updateDisconnectedState();
}

void ModbusClient::resetConnectionState(const quint8 slaveAddress)
{
    ModbusConnectionState& deviceState = m_connectionStatesMap[slaveAddress];
    deviceState.errorsCounter = 0u;
    setDeviceConnected(slaveAddress, &deviceState, true);
}

void ModbusClient::resetAllConnections()
{
    const auto devicesList = m_connectionStatesMap.keys();
    for (auto deviceAddress : devicesList) {
        resetConnectionState(deviceAddress);
    }
}

void ModbusClient::updateDisconnectedState()
{
    if (m_areAllSlavesDisconnected) {
        return;
    }

    bool disconnected = true;
    const auto deviceStatesList = m_connectionStatesMap.values();
    for (auto deviceState : deviceStatesList) {
        if (deviceState.isConnected) {
            disconnected = false;
            break;
        }
    }

    if (disconnected) {
        mbDebug() << Q_FUNC_INFO << "all slaves are disconnected.";
        m_areAllSlavesDisconnected = true;
        emit allSlavesDisconnected();
    }
}

void ModbusClient::onSlaveStateChanged(const quint8 slaveAddress, bool connected)
{
    auto requestsList = m_queryDataMap.values(slaveAddress);
    if (requestsList.isEmpty()) {
        return;
    }

    stopSlavePoll(slaveAddress);
    if (connected) {
        // добавляем в цикл опроса Master'а все запросы для Slave-устройства
        mbDebug() << "slave" << slaveAddress << "came back. Poll is starting...";
        for (auto &request : requestsList) {
            auto query = createReadQuery(slaveAddress, request.dataUnit, request.pollRate);
            m_scheduler->append(query);
        }
    } else if (m_connectionType == Serial) {
        // оставляем только один запрос для проверки связи
        mbDebug() << "slave" << slaveAddress << "waiting for connection ready...";
        auto query = createReadQuery(slaveAddress,
                                     requestsList.first().dataUnit,
                                     requestsList.first().pollRate);
        m_scheduler->append(query);
    }
}

void ModbusClient::sendWriteRequest(const quint8 slaveAddress, const QModbusDataUnit &request)
{
    if (!isConnected()) {
        return;
    }

    quint16 noDelay = 0u;
    auto query = new ModbusWriteQuery(m_client, request, slaveAddress, noDelay, this);
    m_writeQueries.append(query);
    connect(query, &ModbusWriteQuery::receivedError,
            this, &ModbusClient::queryFailed);
    connect(query, &ModbusWriteQuery::receivedError,
            this, &ModbusClient::countErrors);
    connect(query, &ModbusWriteQuery::receivedReply,
            query, &ModbusWriteQuery::deleteLater);
    connect(query, &ModbusWriteQuery::receivedReply,
            this, &ModbusClient::clearWriteQuery);
    connect(query, &ModbusWriteQuery::receivedReply,
            this, &ModbusClient::resetConnectionState);
    connect(query, &ModbusWriteQuery::finished,
            this, &ModbusClient::sendWriteResult);
    connect(query, &ModbusWriteQuery::receivedReply,
            this, &ModbusClient::outputDataUnit);
    m_scheduler->push(query);
}

void ModbusClient::sendWriteResult()
{
    auto query = qobject_cast<ModbusWriteQuery *>(sender());
    if (!query) {
        return;
    }

    auto deviceId = query->slaveAddress();
    auto tableType = query->request().registerType();
    auto startAddress = static_cast<quint16>(query->request().startAddress());
    auto successful = query->hasSucceeded();
    emit writeResultReady(m_deviceNames, deviceId, tableType, startAddress, successful);
}

void ModbusClient::clearWriteQuery()
{
    auto query = qobject_cast<ModbusWriteQuery *>(sender());
    if (query) {
        m_writeQueries.removeAll(query);
    } else {
        // undefined error
    }
}

void ModbusClient::outputDataUnit(const quint8 slaveAddress, const QModbusDataUnit unit)
{
    auto unitAddressLo = static_cast<quint8>(unit.startAddress() & 0xFF);
    auto unitAddressHi = static_cast<quint8>((unit.startAddress() >> 8) & 0xFF);
    auto valueCountLo = static_cast<quint8>(unit.valueCount() & 0xFFU);
    auto valueCountHi = static_cast<quint8>((unit.valueCount() >> 8) & 0xFFU);
    auto functionCode = "ERROR";
    switch (unit.registerType()) {
    case QModbusDataUnit::Coils:
        functionCode = unit.valueCount() > WRITE_REQUEST_SIZE ? "01" : "05";
        break;
    case QModbusDataUnit::DiscreteInputs:
        functionCode = "02";
        break;
    case QModbusDataUnit::HoldingRegisters:
        functionCode = unit.valueCount() > WRITE_REQUEST_SIZE ? "03" : "06";
        break;
    case QModbusDataUnit::InputRegisters:
        functionCode = "04";
        break;
    default:
        break;
    }
    auto replyText = QString("%1 %2 %3 %4 %5 %6 ").arg(slaveAddress, 2, 16, QChar('0'))
            .arg(functionCode)
            .arg(unitAddressHi, 2, 16, QChar('0'))
            .arg(unitAddressLo, 2, 16, QChar('0'))
            .arg(valueCountHi, 2, 16, QChar('0'))
            .arg(valueCountLo, 2, 16, QChar('0'));
    const auto replyValues = unit.values();
    for (quint16 value : replyValues) {
        replyText += QString("%1 %2 ").arg(((value >> 8) & 0xFFU), 2, 16, QChar('0'))
                .arg((value & 0x00FFU), 2, 16, QChar('0'));
    }
    mbDebug() << replyText;
}

void ModbusClient::emitFirstReadDone()
{
    if (m_firstReadDone) {
        return;
    }

    bool allQueriesDone = true;
    for (auto query : qAsConst(m_readQueries)) {
        if (!query->doneOnce()) {
            allQueriesDone = false;
            break;
        }
    }

    if (allQueriesDone) {
        m_firstReadDone = true;
        emit firstReadDone();
    }
}

QModbusClient* ModbusClient::createDevice()
{
    QModbusClient* device = nullptr;
    if (m_connectionType == ModbusConnectionType::Tcp) {
        device = new QModbusTcpClient(this);
    } else {
        device = new QModbusRtuSerialMaster(this);
    }
    return device;
}

void ModbusClient::setConnectionParameters()
{
    auto serialSettings = m_connectionSettings.serial;
    auto tcpSettings = m_connectionSettings.tcp;
    switch (m_connectionType) {
    case ModbusConnectionType::Serial:
        m_client->setConnectionParameter(QModbusDevice::SerialPortNameParameter,
                                         serialSettings.port_name);
        m_client->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,
                                         serialSettings.baud);
        m_client->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,
                                         serialSettings.data_bits);
        m_client->setConnectionParameter(QModbusDevice::SerialParityParameter,
                                         serialSettings.parity);
        m_client->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,
                                         serialSettings.stop_bits);
        mbDebug() << channelId() << serialSettings.baud << m_queryDataMap.first().dataUnit.startAddress();
        break;
    case ModbusConnectionType::Tcp:
        m_client->setConnectionParameter(QModbusDevice::NetworkAddressParameter,
                                         tcpSettings.ip);
        m_client->setConnectionParameter(QModbusDevice::NetworkPortParameter,
                                         tcpSettings.port);
        mbDebug() << channelId() << m_queryDataMap.first().dataUnit.startAddress();
        break;
    case ModbusConnectionType::Unknown:
        throw std::runtime_error("Modbus Source connection type is unknown!");
        break;
    }
    m_client->setTimeout(m_connectionSettings.response_time);
    m_client->setNumberOfRetries(m_connectionSettings.number_of_retries);
    mbDebug() << "timeout:" << m_connectionSettings.response_time;
}

QModbusDataUnit ModbusClient::createRequest(const QModbusDataUnit::RegisterType tableType, const quint16 address, const quint16 value)
{
    quint16 size = 1U;
    int firstIndex = 0;
    auto request = QModbusDataUnit(tableType, address, size);
    request.setValue(firstIndex, value);
    return request;
}

ModbusReadQuery* ModbusClient::createReadQuery(const quint8 slaveAddress, const QModbusDataUnit &request, const quint16 pollRate)
{
    auto query = new ModbusReadQuery(m_client, request, slaveAddress, pollRate, this);
    m_readQueries.append(query);
    connect(query, &ModbusReadQuery::receivedReply,
            this, &ModbusClient::readReady);
    connect(query, &ModbusReadQuery::receivedReply,
            this, &ModbusClient::resetConnectionState);
    connect(query, &ModbusReadQuery::receivedError,
            this, &ModbusClient::queryFailed);
    connect(query, &ModbusReadQuery::receivedError,
            this, &ModbusClient::countErrors);
    connect(query, &ModbusReadQuery::finished,
            this, &ModbusClient::emitFirstReadDone);
    return query;
}

ModbusDeviceInfo ModbusClient::createDeviceInfo(const quint8 deviceAddress)
{
    auto deviceInfo = ModbusDeviceInfo{};
    if (type() == ModbusConnectionType::Serial) {
        deviceInfo = ModbusDeviceInfo(deviceAddress, m_connectionSettings.serial.port_name, m_deviceNames);
    } else if (type() == ModbusConnectionType::Tcp) {
        deviceInfo = ModbusDeviceInfo(deviceAddress,
                                      m_connectionSettings.tcp.ip,
                                      m_connectionSettings.tcp.port,
                                      m_deviceNames);
    }
    return deviceInfo;
}

bool ModbusClient::hasDeviceInfo(const quint8 deviceAddress)
{
    for (auto &deviceInfo : m_deviceInfoList) {
        if (deviceInfo.slaveAddress() == deviceAddress) {
            return true;
        }
    }
    return false;
}

void ModbusClient::tryConnect()
{
    mbDebug() << QString("Trying to connect %1...").arg(description());
    if (m_client) {
        m_client->connectDevice();
    }
}

void ModbusClient::updateToNewPort()
{
    if (m_reconnectCounter >= SEARCH_START_COUNTER) {
        m_reconnectCounter = 0;
        mbDebug() << "Searching for different port...";
        auto newPort = searchNewPort();
        if (!newPort.isEmpty()) {
            mbDebug() << QString("Remapping %1 to %2...")
                         .arg(m_connectionSettings.serial.port_name, newPort);
            m_client->setConnectionParameter(QModbusDevice::SerialPortNameParameter, newPort);
        }
    }
}

QString ModbusClient::searchNewPort()
{
    auto portList = QSerialPortInfo::availablePorts();
    if (portList.isEmpty()) {
        return QString{};
    }

    auto newPortName = QString{};
    auto configuredPortName = m_connectionSettings.serial.port_name;
    for (auto &port : portList) {
        mbDebug() << "found port:" << port.portName() << port.systemLocation()
                  << "is busy:" << port.isNull();
#if defined(Q_OS_WINDOWS)
        auto portName = port.portName();
#elif defined(Q_OS_UNIX)
        auto portName = port.systemLocation();
#endif
        if (!port.isNull() && portName != configuredPortName
                && portName != m_remappedPort )
        {
            m_remappedPort = portName;
            newPortName = portName;
            break;
        }
    }
    return newPortName;
}

void ModbusClient::setDeviceConnected(const quint8 slaveAddress, ModbusConnectionState *deviceState, bool connected)
{
    if (deviceState->isConnected != connected) {
        deviceState->isConnected = connected;
        emit slaveStateChanged(slaveAddress, deviceState->isConnected);
    }
}

ModbusRegistersTableMap ModbusClient::updateRegisters(DeviceBuffersMap &regMap, quint8 slaveAddress, const QModbusDataUnit &reply)
{
    auto updatedRegs = ModbusRegistersTableMap{};
    for (quint16 index = 0; index < reply.valueCount(); index++) {
        auto regAddress = static_cast<quint16>(reply.startAddress() + index);
        QVector<quint16> &slaveRegisters = regMap[slaveAddress][reply.registerType()];
        if (slaveRegisters.at(regAddress) != reply.value(index)) {
            slaveRegisters[regAddress] = reply.value(index);
            updatedRegs[reply.registerType()][regAddress] = reply.value(index);
        }
    }
    return updatedRegs;
}

QList<Modbus::ModbusReadQuery*> ModbusClient::getReadQueriesBySlaveAddress(const quint8 slaveAddress) const
{
    auto slaveQueries = getQueriesBySlaveAddress(m_readQueries, slaveAddress);
    return slaveQueries;
}

std::string ModbusClient::registerTypeToString(const QModbusDataUnit::RegisterType registerType) const
{
    switch (registerType) {
    case QModbusDataUnit::DiscreteInputs: return "DiscreteInputs";
    case QModbusDataUnit::Coils: return "Coils";
    case QModbusDataUnit::InputRegisters: return "InputRegisters";
    case QModbusDataUnit::HoldingRegisters: return "HoldingRegisters";
    default: return "";
    }
}


#ifdef TEST_MODE
void ModbusClient::sendData()
{
    QVector<quint16> array(TEST_REQUEST_SIZE);
    for (quint8 i = 0; i < array.count(); i++) {
        array[i] = (QRandomGenerator::global()->generate() & 0xFFFF);
    }
    writeRegs(18u, array);
}

void ModbusClient::initRequest()
{
    auto config_address = static_cast<quint16>(13u);
    auto enable_post = static_cast<quint16>(0x02u);
    writeReg(config_address, enable_post);
}
#endif

