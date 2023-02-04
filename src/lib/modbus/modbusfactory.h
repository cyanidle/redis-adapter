#ifndef MODBUSFACTORY_H
#define MODBUSFACTORY_H

#include <QObject>
#include <QHash>
#include "modbusclient.h"
#include "redis-adapter/settings/settings.h"

namespace Modbus {
    class RADAPTER_SHARED_SRC ModbusFactory;
}

class Modbus::ModbusFactory : public QObject
{
    Q_OBJECT
public:
    explicit ModbusFactory(QObject *parent = nullptr);
    virtual ~ModbusFactory() override;

    void createDevices(const Settings::ModbusConnectionSettings &connectionSettings);
    ModbusClient* deviceByName(const QString &name);
    ModbusDeviceInfoMap devicesInfo() const;
    QList<ModbusDeviceInfo> deviceInfoByAddress(const quint8 address) const;

    Settings::ModbusConnectionSettings connectionSettings() const;

    void run();
    bool areAllDevicesConnected() const;
signals:
    void dataChanged(quint8 deviceId, const Modbus::ModbusRegistersTableMap &registersMap);
    void writeResultReady(const QStringList &deviceNames,
                          const quint8 deviceId,
                          const QModbusDataUnit::RegisterType tableType,
                          const quint16 startAddress,
                          bool hasSucceeded);
    void connectionChanged(const Modbus::ModbusDeviceInfo &deviceInfo, const bool connected);
    void initRequested(const quint8 deviceId);

public slots:
    void notifyDataChange(quint8 deviceId, const Modbus::ModbusRegistersTableMap &registersMap);
    void changeDeviceData(const QString &deviceName,
                          const quint8 deviceId,
                          const QModbusDataUnit::RegisterType tableType,
                          const Modbus::ModbusRegistersTable &registersTable);

private slots:
    void onConnectionChanged(bool connected);
    void onFirstReadDone();

private:
    QModbusDataUnit queryToDataUnit(const Settings::ModbusQuery &query);
    ModbusQueryMap createQueryMap(const QList<Settings::ModbusSlaveInfo> &slaveInfoList);
    ModbusQueryMap createQueryMap(const Settings::ModbusSlaveInfo &slaveInfo);
    QList<ModbusClient *> createDevicesList(const QList<Settings::ModbusChannelSettings> &channelsList);
    QList<ModbusClient *> createDeviceListByChannel(const Settings::ModbusChannelSettings channel);
    QList<Settings::ModbusSlaveInfo> filterDevicesByType(const QList<Settings::ModbusSlaveInfo> &slaveInfoList,
                                                         const Settings::ModbusConnectionType type);
    QList<QList<Settings::ModbusSlaveInfo>> groupDevicesBySource(const QList<Settings::ModbusSlaveInfo> &devicesList);
    QStringList getRegistersPackNames(const QList<Settings::ModbusSlaveInfo> &slaves);
    QList<Settings::ModbusSlaveInfo> extractDevices(const QList<Settings::ModbusChannelSettings> &channelsList);

    ModbusDeviceInfoMap deviceInfoListToAddressMap(const ModbusDeviceInfoList &deviceInfoList);
    ModbusDeviceInfo getDeviceInfoByAddress(const ModbusDeviceInfoList &deviceInfoList,
                                            const quint8 slaveAddress) const;
    QString getConnectionString(const QString &deviceName, const quint8 deviceId) const;

    Settings::ModbusConnectionSettings m_connectionSettings;
    QHash<QString, ModbusClient*> m_deviceMap;
    ModbusDeviceInfoMap m_deviceInfoMap;
};

#endif // MODBUSFACTORY_H
