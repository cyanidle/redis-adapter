#ifndef MODBUSDEVICESGATE_H
#define MODBUSDEVICESGATE_H

#include <QObject>
#include <QVariant>
#include "redis-adapter/settings/modbussettings.h"
#include "modbusclient.h"

typedef QMultiMap<QString, QVariant> QVariantMultiMap;

class RADAPTER_SHARED_SRC ModbusDevicesGate : public QObject
{
    Q_OBJECT
public:
    struct FullRegisterName {
        QString deviceName;
        QString registerName;
    };

    static ModbusDevicesGate* instance();
    static void init(const Settings::DeviceRegistersInfoMap &devices,
                     const Modbus::ModbusDeviceInfoMap &connectionInfo,
                     QObject *parent);

    QVariantMap packDeviceInfoUnit(const QString &deviceName, const QVariantMap &deviceInfoUnit) const;
    QVariantMultiMap packDevicesData(const QVariantMap &devicesData) const;
    QVariantMap unpackJsonUnit(const QVariantMap &jsonUnit) const;
    QVariantMap unpackJsonDevicesData(const QVariantMap &jsonData) const;
    bool isPackedJsonUnit(const QVariantMap &jsonUnit) const;

    static QList<quint16> splitRegValue(const QVariant &regValue, const QMetaType::Type type);
    static QVariant extractRegValueByType(const QMetaType::Type &type, const QByteArray &dataBytes, const QDataStream::ByteOrder byteOrder);
    FullRegisterName splitFullRegisterName(const QString &fullRegisterName) const;

    void writeJsonToModbusDevice(const QVariant &jsonData, bool *isAbleToWrite = nullptr);
signals:
    void modbusDataChanged(const quint8 deviceId, const QModbusDataUnit::RegisterType tableType, const QList<quint16> &regAddresses);
    void jsonDataReady(const QVariant &jsonData);
    void deviceWriteRequested(const QString &deviceName,
                              const quint8 deviceId,
                              const QModbusDataUnit::RegisterType tableType,
                              const Modbus::ModbusRegistersTable &registersTable);
    void deviceWriteResultReady(const QVariant &jsonData, bool successful);
    void deviceInitRequested(const QString &deviceName);

public slots:
    void readModbusData(const quint8 deviceId,
                        const Modbus::ModbusRegistersTableMap &registersMap);
    void receiveDeviceWriteResult(const QStringList &deviceNames,
                                  const quint8 deviceId,
                                  const QModbusDataUnit::RegisterType tableType,
                                  const quint16 startAddress,
                                  bool hasSucceeded);
    void initModbusDevice(const quint8 deviceId);

private slots:
    void publishModbusAsJson(const quint8 deviceId, const QModbusDataUnit::RegisterType tableType, const QList<quint16> &regAddresses);

private:
    explicit ModbusDevicesGate(const Settings::DeviceRegistersInfoMap &devices,
                               const Modbus::ModbusDeviceInfoMap &connectionInfo,
                               QObject *parent = nullptr);
    static ModbusDevicesGate& prvInstance(const Settings::DeviceRegistersInfoMap &devices = Settings::DeviceRegistersInfoMap{},
                                          const Modbus::ModbusDeviceInfoMap &connectionInfo = Modbus::ModbusDeviceInfoMap{},
                                          QObject *parent = nullptr);

    struct RegisterBytes {
        quint8 high;
        quint8 low;
    };
    struct RegisterData {
        RegisterBytes bytes;
        bool isDword;
        quint16 highWordAddress;
        quint16 lowWordAddress;
        Settings::PackingMode endianess;
        bool isPersistent;
    };
    typedef QMap<quint16, RegisterData> RegistersTable;
    typedef QMap<QModbusDataUnit::RegisterType, RegistersTable> RegistersTableMap;
    typedef QMap<QString /*deviceName*/, RegistersTableMap> DevicesDataMap;

    typedef QMap<quint16 /*regAddress*/, FullRegisterName> AddressNamesTable;
    typedef QMap<QModbusDataUnit::RegisterType, AddressNamesTable> AddressNamesTableMap;
    typedef QMap<quint8 /*deviceId*/, AddressNamesTableMap> DeviceRegNamesMap;
    typedef QMap<quint16, QMetaType::Type> AddressTypesTable;
    typedef QMap<QModbusDataUnit::RegisterType, AddressTypesTable> AddressTypesTableMap;
    typedef QMap<QString /*deviceName*/, AddressTypesTableMap> DeviceRegTypesMap;
    typedef QMap<QString /*deviceName*/, quint8 /*deviceId*/> DeviceIdMap;

    struct WriteRequestInfo {
        QVariantMap rawRequestData;
        quint8 deviceId;
        QModbusDataUnit::RegisterType tableType;
        quint16 startAddress;

        bool isValid() {
            return !rawRequestData.isEmpty() && (deviceId != 0u);
        }
    };

    void initDevicesDataMap();
    void initDevicesRegAddressMap();
    void initDevicesIdNames();

    QVariantMap modbusToJson(const quint8 deviceId, const QModbusDataUnit::RegisterType tableType, const QList<quint16> &regAddresses) const;
    QVariantMap packRegisterData(const QString &regName, const QVariant &regData) const;
    QVariantMap packRegistersData(const QVariantMap &registersData) const;

    QVariantMap createDeviceInfoUnit(const QString &deviceName, const QModbusDataUnit::RegisterType tableType, const QList<quint16> &regAddresses) const;
    QVariantMultiMap joinDeviceInfoUnits(const QVariantList &deviceInfoUnits) const;
    QVariantMap groupJsonByDeviceName(const QVariantMultiMap &jsonUnitData) const;
    bool areValuesValid(const QList<Settings::RegisterInfo> &regInfoList) const;
   QStringList deviceNamesListById(const quint8 deviceId) const;
    quint8 getDeviceInfoUnitDepthLevel(const QString &deviceGroup) const;
    Modbus::ModbusDeviceInfo findConnectionInfo(const QString deviceName) const;
    bool hasDeviceMatch(const QStringList &sourceDevicesList, const QString &targetDeviceName) const;
    QString getFirstDeviceName(const QVariantMap &jsonUnit) const;
    quint8 getFirstDeviceDepthLevel(const QVariantMap &jsonUnit) const;
    void getFirstDeviceDepthLevel(const QVariantMap &jsonUnit, quint16 *recursionLevel) const;
    QVariantMap extractFirstDeviceUnit(const QVariantMap &jsonData, QVariantMap *remainingJsonData = nullptr) const;
    QString getFirstMappedChildName(const QVariantMap &jsonData) const;

    void setDeviceRegistersData(const QString &deviceName, const QModbusDataUnit::RegisterType tableType, const quint16 regAddress, const quint16 regValue);

    RegisterData createRegisterData(const Settings::RegisterInfo &regInfo) const;
    QVariant buildRegValue(const QString &deviceName, const Settings::RegisterInfo &regInfo) const;
    QVariantList buildRegValuesList(const QString &deviceName, const QList<Settings::RegisterInfo> &regInfoList) const;
    RegisterBytes wordToBytes(const quint16 regValue, const QDataStream::ByteOrder byteOrder) const;
    quint16 bytesToWord(const RegisterBytes &regValue) const;
    QByteArray packRegisterData(const QString &deviceName, const QModbusDataUnit::RegisterType tableType, const RegisterData &regData) const;
    QMetaType::Type getTypeByRegisterData(const QString &deviceName, const QModbusDataUnit::RegisterType tableType, const RegisterData &regData) const;
    quint16 calculateHighWordAddress(const quint16 lowWordAddress, const QDataStream::ByteOrder wordOrder) const;
    quint16 calculateLowWordAddress(const quint16 highWordAddress, const QDataStream::ByteOrder wordOrder) const;
    bool hasHighAddress(const RegisterData &regData) const;
    quint16 getSecondWordAddress(const RegisterData &regInfo) const;
    Modbus::ModbusRegistersTableMap createRegistersMap(const QString &deviceName,
                                                       const QModbusDataUnit::RegisterType tableType,
                                                       const quint16 regAddress,
                                                       const QVariant &regValue,
                                                       const RegisterData &regInfo) const;
    QString getFirstDeviceName(const quint8 deviceId, const QModbusDataUnit::RegisterType tableType, const quint16 regAddress) const;
   QStringList getDevicesNameList(const quint8 deviceId, const QModbusDataUnit::RegisterType tableType, const QList<quint16> &regAddresses) const;
    Modbus::ModbusRegistersTableMap buildRegisterDataByAddress(const QString &deviceName, const QModbusDataUnit::RegisterType tableType, const quint16 regAddress, const QVariant &regValue) const;
    Modbus::ModbusRegistersTableMap buildRegisterDataByInfoList(const QString &deviceName,
                                                           const QList<Settings::RegisterInfo> &registersInfo,
                                                           const QVariant &regValue) const;
    Modbus::ModbusRegistersTableMap jsonUnitToRegistersMap(const QVariantMap &jsonUnit) const;

    QVariant getRegValuesByName(const FullRegisterName &name) const;
    AddressNamesTableMap getDeviceAddressMap(const QString &deviceName) const;
    void sortRegistersInfo(QList<Settings::RegisterInfo> &regInfoList) const;

    void enqueueWriteRequest(const QVariantMap &requestData,
                             const quint8 deviceId,
                             const QModbusDataUnit::RegisterType tableType,
                             const quint16 startAddress);
    WriteRequestInfo dequeueWriteRequest(const QStringList &deviceNames,
                                         const quint8 deviceId,
                                         const QModbusDataUnit::RegisterType tableType,
                                         const quint16 startAddress);


    QList<WriteRequestInfo> m_writeRequests;
    Settings::DeviceRegistersInfoMap m_devicesConfigInfo;
    DeviceRegNamesMap m_deviceRegNames;
    DeviceRegTypesMap m_deviceRegTypes;
    DevicesDataMap m_devicesDataMap;
    DeviceIdMap m_deviceIdNames;
    Modbus::ModbusDeviceInfoMap m_devicesConnectionInfo;
};

#endif // FIELDDEVICESGATE_H
