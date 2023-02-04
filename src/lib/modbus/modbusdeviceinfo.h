#ifndef MODBUSDEVICEINFO_H
#define MODBUSDEVICEINFO_H

#include <QString>
#include <QList>
#include <QMap>
#include <QMetaType>
#include "redis-adapter/settings/modbussettings.h"
#include "redis-adapter/settings/settings.h"

namespace Modbus {
    class RADAPTER_SHARED_SRC ModbusDeviceInfo;
    typedef QList<ModbusDeviceInfo> ModbusDeviceInfoList;
    typedef QMultiMap<quint8 /*slaveAddress*/, ModbusDeviceInfo> ModbusDeviceInfoMap;
};

class Modbus::ModbusDeviceInfo
{
public:
    explicit ModbusDeviceInfo(const quint8 address,
                              const QString &serialPort,
                              const QString &sourceName,
                              const QStringList &deviceNames);
    explicit ModbusDeviceInfo(const quint8 address,
                              const QString &ip,
                              const quint16 port,
                              const QString &sourceName,
                              const QStringList &deviceNames);
    explicit ModbusDeviceInfo();

    quint8 slaveAddress() const;
    void setSlaveAddress(const quint8 slaveAddress);

    QString id() const;
    QString connectionString() const;
    void setConnectionString(const QString &serialPort);
    void setConnectionString(const QString &ip, const quint16 port);
    QString sourceName() const;
    QString name() const;
    QStringList relatedDevices() const;
    void setRelatedDevices(const QStringList &deviceNames);
    Settings::ModbusConnectionType type();

    bool isValid() const;
    bool hasDeviceMatch(const QString &deviceToSearch) const;

private:
    QString toId(const quint8 address, const QString &connectionString) const;
    QString toConnectionString(const QString &ip, const quint16 &port) const;

    quint8 m_slaveAddress;
    QString m_connectionString;
    QString m_sourceName;
    QStringList m_relatedDevicesList;
    Settings::ModbusConnectionType m_type;
};

Q_DECLARE_METATYPE(Modbus::ModbusDeviceInfo);

#endif // MODBUSDEVICEINFO_H
