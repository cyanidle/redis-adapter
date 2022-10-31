#include "modbusfactory.h"

using namespace Modbus;
using namespace Settings;

ModbusFactory::ModbusFactory(QObject *parent)
    : QObject(parent),
      m_connectionSettings{},
      m_deviceMap{},
      m_deviceInfoMap{}
{
    qRegisterMetaType<QModbusDataUnit>("QModbusDataUnit");
}

ModbusFactory::~ModbusFactory()
{
    qDeleteAll(m_deviceMap);
    m_deviceMap.clear();
}

void ModbusFactory::createDevices(const ModbusConnectionSettings &connectionSettings)
{
    if (!m_deviceMap.isEmpty()) {
        qDeleteAll(m_deviceMap);
        m_deviceMap.clear();
    }

    const auto deviceList = createDevicesList(connectionSettings.channels);
    auto slavesList = extractDevices(connectionSettings.channels);
    for (auto device : deviceList) {
        m_deviceMap.insert(device->channelId(), device);
        for (auto &slave : slavesList) {
            if (device->sourceName() != slave.source.name) {
                continue;
            }
            auto deviceInfo = getDeviceInfoByAddress(device->devicesInfo(), slave.address);
            if (!deviceInfo.isValid()) {
                continue;
            }
            deviceInfo.setRelatedDevices(slave.registers);
            if (deviceInfo.isValid()) {
                m_deviceInfoMap.insert(slave.address, deviceInfo);
            }
        }
    }

    m_connectionSettings = connectionSettings;
}

ModbusClient* ModbusFactory::deviceByName(const QString &name)
{
    return m_deviceMap.value(name);
}

ModbusDeviceInfoMap ModbusFactory::devicesInfo() const
{
    return m_deviceInfoMap;
}

QList<ModbusDeviceInfo> ModbusFactory::deviceInfoByAddress(const quint8 address) const
{
    return m_deviceInfoMap.values(address);
}

ModbusConnectionSettings ModbusFactory::connectionSettings() const
{
    return m_connectionSettings;
}

void ModbusFactory::run()
{
    const auto devices = m_deviceMap.values();
    for (auto device : devices) {
        device->run();
    }
}

bool ModbusFactory::areAllDevicesConnected() const
{
    bool connected = true;
    const auto devices = m_deviceMap.values();
    for (auto device : devices) {
        if (!device->isConnected()) {
            connected = false;
            break;
        }
    }
    return connected;
}

void ModbusFactory::notifyDataChange(quint8 deviceId, const ModbusRegistersTableMap &registersMap)
{
    emit dataChanged(deviceId, registersMap);
}

void ModbusFactory::changeDeviceData(const QString &deviceName, const quint8 deviceId, 
    const QModbusDataUnit::RegisterType tableType, const ModbusRegistersTable &registersTable)
{
    auto channelId = getConnectionString(deviceName, deviceId);
    if (channelId.isEmpty()) {
        return;
    }
    auto device = m_deviceMap.value(channelId);
    if (device) {
        device->changeData(deviceId, tableType, registersMap);
    }
}

void ModbusFactory::onConnectionChanged(bool connected)
{
    auto client = qobject_cast<ModbusClient*>(sender());
    if (!client) {
        return;
    }

    const auto devicesIdList = client->devicesIdList();
    for (auto deviceId : devicesIdList) {
        auto devices = deviceInfoByAddress(deviceId);
        auto devicesNames = QStringList{};
        for (auto &device : devices) {
            devicesNames.append(device.relatedDevices());
        }
        emit connectionChanged(devicesNames, connected);
    }
}

void ModbusFactory::onFirstReadDone()
{
    auto client = qobject_cast<ModbusClient*>(sender());
    if (!client) {
        return;
    }

    const auto devicesIdList = client->devicesIdList();
    for (auto deviceId : devicesIdList) {
        emit initRequested(deviceId);
    }
}

QModbusDataUnit ModbusFactory::queryToDataUnit(const Settings::ModbusQuery &query)
{
    auto mbQuery = QModbusDataUnit(query.type, query.reg_index, query.reg_count);
    return mbQuery;
}

ModbusQueryMap ModbusFactory::createQueryMap(const QList<Settings::ModbusSlaveInfo> &slaveInfoList)
{
    auto queryMap = ModbusQueryMap{};
    for (auto &slave : slaveInfoList) {
        auto slaveQueryMap = createQueryMap(slave);
        queryMap.unite(slaveQueryMap);
    }
    return queryMap;
}

ModbusQueryMap ModbusFactory::createQueryMap(const Settings::ModbusSlaveInfo &slaveInfo)
{
    auto queryMap = ModbusQueryMap{};
    // QMap's insertMulti places last item to the top. Thus, using reverse order.
    for (auto query = slaveInfo.queries.rbegin();
         query != slaveInfo.queries.rend();
         query++)
    {
        auto queryInfo = ModbusQueryInfo{ queryToDataUnit(*query), query->poll_rate };
        queryMap.insert(slaveInfo.address, queryInfo);
    }
    return queryMap;
}

QList<ModbusClient *> ModbusFactory::createDevicesList(const QList<ModbusChannelSettings> &channelsList)
{
    auto channelsDevices = extractDevices(channelsList);
    const auto groupedDevices = groupDevicesBySource(channelsDevices);
    auto devicesList = QList<ModbusClient *>{};
    for (auto devicesGroup : groupedDevices) {
        auto queryMap = createQueryMap(devicesGroup);
        auto connectionSettings = devicesGroup.first().source;
        auto associatedDevices = getRegistersPackNames(devicesGroup);
        auto device = new ModbusClient(connectionSettings, queryMap, associatedDevices, this);
        connect(device, &ModbusClient::dataChanged,
                this, &ModbusFactory::notifyDataChange);
        connect(device, &ModbusClient::writeResultReady,
                this, &ModbusFactory::writeResultReady);
        connect(device, &ModbusClient::connectionChanged,
                this, &ModbusFactory::onConnectionChanged);
        connect(device, &ModbusClient::firstReadDone,
                this, &ModbusFactory::onFirstReadDone);
        devicesList.append(device);
    }
    return devicesList;
}

QList<ModbusClient *> ModbusFactory::createDeviceListByChannel(const Settings::ModbusChannelSettings channel)
{
    auto deviceList = QList<ModbusClient *>{};
    auto serialDevices = filterDevicesByType(channel.slaves, ModbusConnectionType::Serial);
    const auto uniquePortGroups = groupDevicesBySource(serialDevices);
    for (auto deviceGroup : uniquePortGroups) {
        auto queryMap = createQueryMap(deviceGroup);
        auto connectionSettings = deviceGroup.first().source;
        auto associatedDevices = getRegistersPackNames(deviceGroup);
        auto device = new ModbusClient(connectionSettings, queryMap, associatedDevices, this);
        connect(device, &ModbusClient::dataChanged,
                this, &ModbusFactory::notifyDataChange);
        connect(device, &ModbusClient::writeResultReady,
                this, &ModbusFactory::writeResultReady);
        deviceList.append(device);
    }

    auto tcpDevices = filterDevicesByType(channel.slaves, ModbusConnectionType::Tcp);
    for (auto &tcpDevice : tcpDevices) {
        auto queryMap = createQueryMap(tcpDevice);
        if (queryMap.isEmpty()) {
            continue;
        }

        auto device = new ModbusClient(tcpDevice.source, queryMap, tcpDevice.registers, this);
        connect(device, &ModbusClient::dataChanged,
                this, &ModbusFactory::notifyDataChange);
        connect(device, &ModbusClient::writeResultReady,
                this, &ModbusFactory::writeResultReady);
        deviceList.append(device);
    }

    return deviceList;
}

QList<ModbusSlaveInfo> ModbusFactory::filterDevicesByType(const QList<ModbusSlaveInfo> &slaveInfoList, const ModbusConnectionType type)
{
    auto serialDevices = QList<ModbusSlaveInfo>{};
    for (auto &slave : slaveInfoList) {
        if (slave.source.type == type) {
            serialDevices.append(slave);
        }
    }
    return serialDevices;
}

QList<QList<ModbusSlaveInfo>> ModbusFactory::groupDevicesBySource(const QList<ModbusSlaveInfo> &devicesList)
{
    auto groups = QList<QList<ModbusSlaveInfo>>{};
    for (auto &device : devicesList) {
        qint8 foundGroup = -1;
        auto currentSource = device.source.name;
        for (qint8 groupIndex = 0u; groupIndex < groups.count(); groupIndex++) {
            auto oldGroup = groups.at(groupIndex);
            auto oldSource = oldGroup.first().source.name;
            if (currentSource == oldSource) {
                foundGroup = groupIndex;
                break;
            }
        }
        if (foundGroup >= 0) {
            groups[foundGroup].append(device);
        } else {
            auto portGroup = QList<ModbusSlaveInfo>{};
            portGroup.append(device);
            groups.append(portGroup);
        }
    }

    return groups;
}

QStringList ModbusFactory::getRegistersPackNames(const QList<ModbusSlaveInfo> &slaves)
{
    auto registersPacks = QStringList{};
    for (auto &slave : slaves) {
        registersPacks.append(slave.registers);
    }
    return registersPacks;
}

QList<ModbusSlaveInfo> ModbusFactory::extractDevices(const QList<ModbusChannelSettings> &channelsList)
{
    auto devicesList = QList<ModbusSlaveInfo>{};
    for (auto &channel : channelsList) {
        devicesList.append(channel.slaves);
    }
    return devicesList;
}

ModbusDeviceInfoMap ModbusFactory::deviceInfoListToAddressMap(const ModbusDeviceInfoList &deviceInfoList)
{
    auto deviceInfoMap = ModbusDeviceInfoMap{};
    for (auto &deviceInfo : deviceInfoList) {
        deviceInfoMap.insert(deviceInfo.slaveAddress(), deviceInfo);
    }
    return deviceInfoMap;
}

ModbusDeviceInfo ModbusFactory::getDeviceInfoByAddress(const ModbusDeviceInfoList &deviceInfoList, const quint8 slaveAddress) const
{
    for (auto deviceInfo : deviceInfoList) {
        if (deviceInfo.slaveAddress() == slaveAddress) {
            return deviceInfo;
        }
    }
    return ModbusDeviceInfo{};
}

QString ModbusFactory::getConnectionString(const QString &deviceName, const quint8 deviceId) const
{
    auto devicesList = deviceInfoByAddress(deviceId);
    auto connectionString = QString{};
    for (auto &device : devicesList) {
        if (device.hasDeviceMatch(deviceName)) {
            connectionString = device.connectionString();
        }
    }
    return connectionString;
}
