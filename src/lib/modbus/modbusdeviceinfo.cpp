#include "modbusdeviceinfo.h"

using namespace Modbus;

ModbusDeviceInfo::ModbusDeviceInfo(const quint8 address,
                                   const QString &serialPort,
                                   const QString &sourceName,
                                   const QStringList &deviceNames)
    : m_slaveAddress(address),
      m_connectionString(serialPort),
      m_sourceName(sourceName),
      m_relatedDevicesList(deviceNames),
      m_type(Settings::ModbusConnectionType::Serial)
{
}

ModbusDeviceInfo::ModbusDeviceInfo(const quint8 address,
                                   const QString &ip,
                                   const quint16 port,
                                   const QString &sourceName,
                                   const QStringList &deviceNames)
    : m_slaveAddress(address),
      m_sourceName(sourceName),
      m_relatedDevicesList(deviceNames),
      m_type(Settings::ModbusConnectionType::Tcp)
{
    m_connectionString = toConnectionString(ip, port);
}

ModbusDeviceInfo::ModbusDeviceInfo() : ModbusDeviceInfo(0u, QString{}, QString{}, QStringList{})
{
}

quint8 ModbusDeviceInfo::slaveAddress() const
{
    return m_slaveAddress;
}

void ModbusDeviceInfo::setSlaveAddress(const quint8 slaveAddress)
{
    if (slaveAddress > 0u) {
        m_slaveAddress = slaveAddress;
    }
}

QString ModbusDeviceInfo::id() const
{
    auto deviceId = toId(m_slaveAddress, connectionString());
    return deviceId;
}

QString ModbusDeviceInfo::connectionString() const
{
    return m_connectionString;
}

void ModbusDeviceInfo::setConnectionString(const QString &serialPort)
{
    if (!serialPort.isEmpty()
            && (type() == Settings::ModbusConnectionType::Serial))
    {
        m_connectionString = serialPort;
    }
}

void ModbusDeviceInfo::setConnectionString(const QString &ip, const quint16 port)
{
    if (!ip.isEmpty() && (port > 0u)
            && (type() == Settings::ModbusConnectionType::Tcp))
    {
        m_connectionString = toConnectionString(ip, port);
    }
}

QString ModbusDeviceInfo::sourceName() const
{
    return m_sourceName;
}

QString ModbusDeviceInfo::name() const
{
    auto name = QStringList{ sourceName(), QString::number(slaveAddress()) }
            .join(":");
    return name;
}

QStringList ModbusDeviceInfo::relatedDevices() const
{
    return m_relatedDevicesList;
}

void ModbusDeviceInfo::setRelatedDevices(const QStringList &deviceNames)
{
    if (!deviceNames.isEmpty()) {
        m_relatedDevicesList = deviceNames;
    }
}

Settings::ModbusConnectionType ModbusDeviceInfo::type()
{
    return m_type;
}

bool ModbusDeviceInfo::isValid() const
{
    return !m_relatedDevicesList.isEmpty() && !connectionString().isEmpty()
            && m_slaveAddress != 0u;
}

bool ModbusDeviceInfo::hasDeviceMatch(const QString &deviceToSearch) const
{
    bool hasMatch = false;
    for (auto &sourceDevice : m_relatedDevicesList) {
        if (sourceDevice.contains("*")) {
            auto deviceNameFilter = sourceDevice.split("*").first();
            hasMatch = deviceToSearch.contains(deviceNameFilter);
        } else {
            hasMatch = sourceDevice == deviceToSearch;
        }
        if (hasMatch) {
            break;
        }
    }
    return hasMatch;
}


QString ModbusDeviceInfo::toId(const quint8 address, const QString &connectionString) const
{
    return QString("%1@%2").arg(address).arg(connectionString);
}

QString ModbusDeviceInfo::toConnectionString(const QString &ip, const quint16 &port) const
{
    return QString("%1:%2").arg(ip).arg(port);
}
