#include "modbusdevicesgate.h"
#include <QDataStream>
#include <QSysInfo>
#include "redis-adapter/radapterlogging.h"
#include "redis-adapter/include/modbuskeys.h"

using namespace Modbus;
using namespace Settings;

#define DWORD_SIZE      2

ModbusDevicesGate::ModbusDevicesGate(const DeviceRegistersInfoMap &devices,
                                     const ModbusDeviceInfoMap &connectionInfo,
                                     QObject *parent)
    : QObject(parent),
      m_writeRequests{},
      m_devicesConfigInfo(devices),
      m_devicesConnectionInfo(connectionInfo)
{
    initDevicesDataMap();
    initDevicesRegAddressMap();
    connect(this, &ModbusDevicesGate::modbusDataChanged,
            this, &ModbusDevicesGate::publishModbusAsJson);
}

ModbusDevicesGate *ModbusDevicesGate::instance()
{
    return &prvInstance();
}

ModbusDevicesGate &ModbusDevicesGate::prvInstance(const Settings::DeviceRegistersInfoMap &devices, const Modbus::ModbusDeviceInfoMap &connectionInfo, QObject *parent)
{
    static ModbusDevicesGate gate(devices, connectionInfo, parent);
    return gate;
}

void ModbusDevicesGate::init(const Settings::DeviceRegistersInfoMap &devices,
                             const Modbus::ModbusDeviceInfoMap &connectionInfo,
                             QObject *parent)
{
    prvInstance(devices, connectionInfo, parent);
}

void ModbusDevicesGate::initDevicesDataMap()
{
    for (auto device = m_devicesConfigInfo.begin();
         device != m_devicesConfigInfo.end();
         device++)
    {
        const auto regInfoList = device->values();
        auto registersTableMap = RegistersTableMap{};
        auto registersTypesMap = AddressTypesTableMap{};
        for (auto &regInfo : regInfoList) {
            auto regData = createRegisterData(regInfo);
            registersTableMap[regInfo.table][regInfo.index] = regData;
            registersTypesMap[regInfo.table][regInfo.index] = regInfo.type;
        }
        m_devicesDataMap.insert(device.key(), registersTableMap);
        m_deviceRegTypes.insert(device.key(), registersTypesMap);
    }
}

void ModbusDevicesGate::initDevicesRegAddressMap()
{
    initDevicesIdNames();
    for (auto device = m_devicesConfigInfo.begin();
         device != m_devicesConfigInfo.end();
         device++)
    {
        auto deviceId = m_deviceIdNames.value(device.key());
        auto deviceRegisters = device.value();
        AddressNamesTableMap &addressNames = m_deviceRegNames[deviceId];
        for (auto deviceReg = deviceRegisters.begin();
             deviceReg != deviceRegisters.end();
             deviceReg++)
        {
            addressNames[deviceReg.value().table][deviceReg.value().index] = FullRegisterName{device.key(), deviceReg.key()};
        }
    }
}

void ModbusDevicesGate::initDevicesIdNames()
{
    const auto deviceNamesList = m_devicesConfigInfo.keys();
    for (auto &deviceName : deviceNamesList) {
        auto matchedDevice = findConnectionInfo(deviceName);
        if (matchedDevice.isValid()) {
            m_deviceIdNames.insert(deviceName, matchedDevice.slaveAddress());
        }
    }
}

void ModbusDevicesGate::readModbusData(const quint8 deviceId, const ModbusRegistersTableMap &registersMap)
{
    if (!m_devicesConnectionInfo.contains(deviceId)) {
        return;
    }

    auto tablesList = QVector<QModbusDataUnit::RegisterType>{};
    // TODO: проверить ситуацию, когда данные получены от plc, plc.post.1 одновременно
    for (auto regTable = registersMap.begin(); regTable != registersMap.end(); regTable++) {
        auto regTableType = regTable.key();
        auto registers = regTable.value();
        for (auto reg = registers.begin(); reg != registers.end(); reg++) {
            auto regAddress = reg.key();
            auto deviceName = getFirstDeviceName(deviceId, regTableType, regAddress);
            setDeviceRegistersData(deviceName, regTableType, regAddress, reg.value());
        }
        if (!tablesList.contains(regTableType)) {
            tablesList.append(regTableType);
        }
    }

    // запросы Modbus Master'а идут отдельно для каждого типа регистров,
    // поэтому изменения фиксируются отдельно для каждой таблицы
    for (auto tableType : qAsConst(tablesList)) {
        auto regAddressList = registersMap[tableType].keys();
        emit modbusDataChanged(deviceId, tableType, regAddressList);
    }
}

void ModbusDevicesGate::writeJsonToModbusDevice(const QVariant &jsonData, bool *isAbleToWrite)
{
    auto jsonDict = jsonData.toMap();
    if (jsonDict.isEmpty()) {
        return;
    }

    auto unpackedJsonDevices = jsonDict;
    if (isPackedJsonUnit(jsonDict)) {
        unpackedJsonDevices = unpackJsonDevicesData(jsonDict);
    }
    bool hasRegistersToWrite = false;
    for (auto jsonUnit = unpackedJsonDevices.begin();
         jsonUnit != unpackedJsonDevices.end();
         jsonUnit++)
    {
        auto jsonDevice = QVariantMap{ { jsonUnit.key(), jsonUnit.value() } };
        auto registersMap = jsonUnitToRegistersMap(jsonDevice);
        auto writeRegisters = registersMap.value(QModbusDataUnit::HoldingRegisters);
        if (!writeRegisters.isEmpty()) {
            auto deviceName = getFirstDeviceName(jsonDevice);
            auto deviceId = m_deviceIdNames.value(deviceName);
            enqueueWriteRequest(jsonDevice, deviceId, writeRegisters.firstKey());
            emit deviceWriteRequested(deviceName, deviceId, registersMap);
            hasRegistersToWrite = true;
        }
    }
    if (isAbleToWrite) {
        *isAbleToWrite = hasRegistersToWrite;
    }
}

void ModbusDevicesGate::receiveDeviceWriteResult(const QStringList &deviceNames, const quint8 deviceId, const quint16 startAddress, bool hasSucceeded)
{
    auto writeRequest = dequeueWriteRequest(deviceNames, deviceId, startAddress);
    if (writeRequest.isValid()) {
        emit deviceWriteResultReady(writeRequest.rawRequestData, hasSucceeded);
    }
}

void ModbusDevicesGate::publishModbusAsJson(const quint8 deviceId, const QModbusDataUnit::RegisterType tableType, const QList<quint16> &regAddresses)
{
    auto jsonData = modbusToJson(deviceId, tableType, regAddresses);
    if (jsonData.isEmpty()) {
        return;
    }
    emit jsonDataReady(jsonData);
}

void ModbusDevicesGate::initModbusDevice(const quint8 deviceId)
{
    auto deviceNames = deviceNamesListById(deviceId);
    for (auto &deviceName : deviceNames) {
        emit deviceInitRequested(deviceName);
    }
}

QVariantMap ModbusDevicesGate::modbusToJson(const quint8 deviceId, const QModbusDataUnit::RegisterType tableType, const QList<quint16> &regAddresses) const
{
    // creates the map of registers grouped by DeviceNames (see: json_example.json)
    auto packedJsonUnit = QVariantMap{};
    const auto deviceNames = getDevicesNameList(deviceId, tableType, regAddresses);
    if (deviceNames.isEmpty()) {
        return QVariantMap{};
    }

    auto jsonData = QVariantMultiMap{};
    for (auto &deviceName : deviceNames) {
        auto jsonUnit = createDeviceInfoUnit(deviceName, tableType, regAddresses);
        if (!jsonUnit.isEmpty()) {
            packedJsonUnit = packDeviceInfoUnit(deviceName, jsonUnit);
        }

        auto jsonDeviceData = groupJsonByDeviceName(packedJsonUnit);
        jsonData.unite(jsonDeviceData);
    }
    auto jsonDataMap = groupJsonByDeviceName(jsonData);
    return jsonDataMap;
}

QVariantMap ModbusDevicesGate::createDeviceInfoUnit(const QString &deviceName, const QModbusDataUnit::RegisterType tableType, const QList<quint16> &regAddresses) const
{
    auto deviceAddressMap = getDeviceAddressMap(deviceName);
    if (deviceAddressMap.isEmpty()) {
        return QVariantMap{};
    }

    auto deviceUnit = QVariantMap{};
    for (auto address : regAddresses) {
        auto name = deviceAddressMap.value(tableType).value(address);
        if (name.deviceName != deviceName)
            continue;

        auto dataValue = getRegValuesByName(name);
        if (dataValue.isValid()) {
            deviceUnit.insert(name.registerName, dataValue);
        }
    }
    return deviceUnit;
}

bool ModbusDevicesGate::areValuesValid(const QList<RegisterInfo> &regInfoList) const
{
    auto allValuesValid = true;
    for (auto &regInfo : regInfoList) {
        QMetaType regType(regInfo.type);
        if (!regType.isValid()) {
            allValuesValid = false;
            break;
        }
    }
    return allValuesValid;
}

QStringList ModbusDevicesGate::deviceNamesListById(const quint8 deviceId) const
{
    auto deviceNames = QStringList{};
    for (auto deviceInfo = m_deviceIdNames.begin();
         deviceInfo != m_deviceIdNames.end();
         deviceInfo++)
    {
        auto deviceName = deviceInfo.key();
        if ((deviceInfo.value() == deviceId)
                && !deviceNames.contains(deviceName))
        {
            deviceNames.append(deviceName);
        }
    }
    return deviceNames;
}

quint8 ModbusDevicesGate::getDeviceInfoUnitDepthLevel(const QString &deviceGroup) const
{
    quint8 depthLevel = 0u;
    const auto deviceNamesList = m_devicesConfigInfo.keys();
    for (auto &deviceName : deviceNamesList) {
        if (deviceName.contains(deviceGroup)) {
            depthLevel = static_cast<quint8>(deviceName.split(".").count());
            break;
        }
    }
    return depthLevel;
}

Modbus::ModbusDeviceInfo ModbusDevicesGate::findConnectionInfo(const QString deviceName) const
{
    auto matchedDevice = Modbus::ModbusDeviceInfo{};
    for (auto &deviceInfo : m_devicesConnectionInfo) {
        if (deviceInfo.hasDeviceMatch(deviceName)) {
            matchedDevice = deviceInfo;
            break;
        }
    }
    return matchedDevice;
}

bool ModbusDevicesGate::hasDeviceMatch(const QStringList &sourceDevicesList, const QString &targetDeviceName) const
{
    bool hasMatch = false;
    for (auto &sourceDevice : sourceDevicesList) {
        if (sourceDevice.contains("*")) {
            auto deviceNameFilter = sourceDevice.split("*").first();
            hasMatch = targetDeviceName.contains(deviceNameFilter);
        } else {
            hasMatch = sourceDevice == targetDeviceName;
        }
        if (hasMatch) {
            break;
        }
    }
    return hasMatch;
}

QString ModbusDevicesGate::getFirstDeviceName(const QVariantMap &jsonUnit) const
{
    auto depthLevel = getFirstDeviceDepthLevel(jsonUnit);
    auto deviceName = QString{};
    auto currentLevel = jsonUnit;
    auto separator = QString(".");
    for (quint8 depthIndex = 1u; depthIndex <= depthLevel; depthIndex++) {
        deviceName += currentLevel.firstKey() + separator;
        currentLevel = currentLevel.first().toMap();
    }

    if (!deviceName.isEmpty()) {
        deviceName.chop(separator.size());
    }
    return deviceName;
}

quint8 ModbusDevicesGate::getFirstDeviceDepthLevel(const QVariantMap &jsonUnit) const
{
    auto depthLevel = quint16{};
    getFirstDeviceDepthLevel(jsonUnit, &depthLevel);
    return static_cast<quint8>(depthLevel);
}

void ModbusDevicesGate::getFirstDeviceDepthLevel(const QVariantMap &jsonUnit, quint16 *recursionLevel) const
{
    auto currentLevel = jsonUnit.first();
    if (!currentLevel.canConvert(QMetaType::QVariantMap)) {
        return;
    }

    bool hasNextLevelUnit = currentLevel.toMap().first().canConvert(QMetaType::QVariantMap);
    if (hasNextLevelUnit) {
        getFirstDeviceDepthLevel(currentLevel.toMap(), recursionLevel);
    }
    bool isArray = !hasNextLevelUnit && currentLevel.toMap().firstKey().toUInt();
    if (!isArray) {
        ++(*recursionLevel);
    }
    return;
}

QVariantMap ModbusDevicesGate::extractFirstDeviceUnit(const QVariantMap &jsonData, QVariantMap *remainingJsonData) const
{
    /* TODO: извлечь все данные, относящиеся к getDeviceName(jsonData).
     * Оставшиеся данные передать в remainingJsonData.
     * Завести отдельный метод unpack для распаковки структуры из нескольких устройств в словарь <имя устройства, регистры>
     */
    auto remainingData = jsonData;
    auto firstMappedChildName = getFirstMappedChildName(remainingData);
    if (firstMappedChildName.isEmpty()) {
        if (remainingJsonData) {
            *remainingJsonData = QVariantMap{};
        }
        return jsonData;
    }

    auto currentLevelFirstUnit = remainingData.take(firstMappedChildName);
    auto nextLevelRemainingData = QVariantMap{};
    auto nextLevelFirstUnit = extractFirstDeviceUnit(currentLevelFirstUnit.toMap(), &nextLevelRemainingData);

    if (!nextLevelRemainingData.isEmpty()) {
        remainingData.insert(firstMappedChildName, nextLevelRemainingData);
    }
    if (remainingJsonData) {
        *remainingJsonData = remainingData;
    }

    auto firstDeviceUnit = QVariantMap{ { firstMappedChildName, nextLevelFirstUnit } };
    return firstDeviceUnit;
}

QString ModbusDevicesGate::getFirstMappedChildName(const QVariantMap &jsonData) const
{
    auto mappedChildName = QString{};
    for (auto jsonItem = jsonData.begin(); jsonItem != jsonData.end(); jsonItem++) {
        if (jsonItem->canConvert(QMetaType::QVariantMap)) {
            mappedChildName = jsonItem.key();
            break;
        }
    }
    return mappedChildName;
}

QVariantMap ModbusDevicesGate::packDeviceInfoUnit(const QString &deviceName, const QVariantMap &deviceInfoUnit) const
{
    auto deviceNameLevels = deviceName.split(".");
    auto reversedDepthLevel = deviceNameLevels.rbegin();
    auto packedUnitData = packRegistersData(deviceInfoUnit);
    auto jsonUnit = QVariantMap{{*reversedDepthLevel, packedUnitData}};
    reversedDepthLevel++;
    while (reversedDepthLevel != deviceNameLevels.rend())
    {
        jsonUnit = QVariantMap{{*reversedDepthLevel, jsonUnit}};
        reversedDepthLevel++;
    }
    return jsonUnit;
}

QVariantMap ModbusDevicesGate::packRegisterData(const QString &regName, const QVariant &regData) const
{
    auto registerNameLevels = regName.split(".");
    bool isRegisterArray = registerNameLevels.count() == 2;
    auto packedRegister = QVariantMap{};
    if (isRegisterArray) {
        packedRegister.insert(registerNameLevels.first(), QVariantMap{{registerNameLevels.last(), regData}});
    } else {
        packedRegister.insert(regName, regData);
    }
    return packedRegister;
}

QVariantMap ModbusDevicesGate::packRegistersData(const QVariantMap &registersData) const
{
    auto packedRegisters = QVariantMap{};
    for (auto registerUnit = registersData.begin();
         registerUnit != registersData.end();
         registerUnit++)
    {
        auto packedData = packRegisterData(registerUnit.key(), registerUnit.value());
        auto previousData = packedRegisters.value(packedData.firstKey());
        if (packedData.first().canConvert<QVariantMap>() && previousData.isValid())
        {
            auto joinedData = QVariantMap{ packedData.first().toMap() };
            joinedData.insert(previousData.toMap());
            packedData.first() = joinedData;
        }
        packedRegisters.insert(packedData);
    }
    return packedRegisters;
}

QVariantMultiMap ModbusDevicesGate::packDevicesData(const QVariantMap &devicesData) const
{
    auto packedDevicesData = QVariantMultiMap{};
    for (auto device = devicesData.begin();
         device != devicesData.end();
         device++)
    {
        auto deviceName = device.key();
        auto packedUnit = packDeviceInfoUnit(deviceName, device.value().toMap());
        packedDevicesData.unite(packedUnit);
    }
    return packedDevicesData;
}

QVariantMap ModbusDevicesGate::unpackJsonUnit(const QVariantMap &jsonUnit) const
{
    // TODO: проверить, как реагирует на ключ модуля, от которого пришёл запрос на запись
    auto depthLevel = getFirstDeviceDepthLevel(jsonUnit);
    if (depthLevel <= 1u) {
        return jsonUnit;
    }

    auto deviceInfoUnit = jsonUnit;
    for (quint8 depthIndex = 1u; depthIndex < depthLevel; depthIndex++) {
        auto nextLevelMap = deviceInfoUnit.first().toMap();
        auto nextLevelName = nextLevelMap.firstKey();
        auto deviceName = QString("%1.%2").arg(deviceInfoUnit.firstKey(), nextLevelName);
        deviceInfoUnit = QVariantMap{ { deviceName, nextLevelMap.first() } };
    }
    return deviceInfoUnit;
}

QVariantMap ModbusDevicesGate::unpackJsonDevicesData(const QVariantMap &jsonData) const
{
    auto devicesDataMap = QVariantMap{};
    auto remainingJsonData = jsonData;
    while (!remainingJsonData.isEmpty()) {
        auto jsonUnit = extractFirstDeviceUnit(remainingJsonData, &remainingJsonData);
        auto deviceData = unpackJsonUnit(jsonUnit);
        devicesDataMap.insert(deviceData);
    }
    return devicesDataMap;
}

QVariantMultiMap ModbusDevicesGate::joinDeviceInfoUnits(const QVariantList &deviceInfoUnits) const
{
    auto deviceUnitData = QVariantMultiMap{};
    for (auto &unitData : deviceInfoUnits) {
        deviceUnitData.unite(unitData.toMap());
    }
    return deviceUnitData;
}

QVariantMap ModbusDevicesGate::groupJsonByDeviceName(const QVariantMultiMap &jsonUnitData) const
{
    if (jsonUnitData.uniqueKeys().count() == jsonUnitData.count()) {
        return jsonUnitData;
    }

    if (!jsonUnitData.first().canConvert(QMetaType::QVariantMap)) {
        return jsonUnitData;
    }

    auto groupedJson = QVariantMap{};
    for (auto &foundDeviceName : jsonUnitData.uniqueKeys()) {
        auto unitDataItems = jsonUnitData.values(foundDeviceName);
        if (unitDataItems.count() > 1) {
            auto subDeviceUnitData = joinDeviceInfoUnits(unitDataItems);
            groupedJson.insert(foundDeviceName,
                               groupJsonByDeviceName(subDeviceUnitData));
        } else {
            groupedJson.insert(foundDeviceName, unitDataItems.first());
        }
    }
    return groupedJson;
}

ModbusDevicesGate::RegisterData ModbusDevicesGate::createRegisterData(const Settings::RegisterInfo &regInfo) const
{
    // Single word registers should have the Type specified.
    // So expecting only high or low words of DWORD values, when checking Type validity.
    auto isSingleWord = regInfo.type == QMetaType::UShort;
    auto regData = RegisterData{};
    regData.endianess = regInfo.endianess;
    regData.isDword = !isSingleWord;
    regData.isPersistent = regInfo.is_persistent;
    if (regData.isDword) {
        QMetaType regType(regInfo.type);
        auto wordOrder = regInfo.endianess.word_order;
        auto isLowWord = wordOrder == QDataStream::BigEndian
                ? !regType.isValid()
                : regType.isValid();
        if (isLowWord) {
            regData.highWordAddress = calculateHighWordAddress(regInfo.index, wordOrder);
            regData.lowWordAddress = 0u;
        } else {
            regData.highWordAddress = 0u;
            regData.lowWordAddress = calculateLowWordAddress(regInfo.index, wordOrder);
        }
    }
    return regData;
}

QVariant ModbusDevicesGate::buildRegValue(const QString &deviceName, const Settings::RegisterInfo &regInfo) const
{
    auto deviceRegisters = m_devicesDataMap.value(deviceName).value(regInfo.table);
    if (!deviceRegisters.contains(regInfo.index))
        return QVariant{};

    auto regData = deviceRegisters.value(regInfo.index);
    auto bytes = packRegisterData(deviceName, regInfo.table, regData);

    auto type = QMetaType(regInfo.type).isValid() ? regInfo.type
                                                  : getTypeByRegisterData(deviceName, regInfo.table, regData);
    auto regValue = extractRegValueByType(type, bytes, regInfo.endianess.byte_order);
    return regValue;
}

QVariantList ModbusDevicesGate::buildRegValuesList(const QString &deviceName, const QList<Settings::RegisterInfo> &regInfoList) const
{
    auto regValues = QVariantList{};
    auto sortedRegList = regInfoList;
    sortRegistersInfo(sortedRegList);
    for (auto &regInfo : qAsConst(sortedRegList)) {
        auto regValue = buildRegValue(deviceName, regInfo);
        regValues.append(regValue);
    }
    return regValues;
}

ModbusDevicesGate::RegisterBytes ModbusDevicesGate::wordToBytes(const quint16 regValue, const QDataStream::ByteOrder byteOrder) const
{
    auto bytes = QByteArray{};
    QDataStream byteStream{ &bytes, QIODevice::WriteOnly };
    byteStream.setByteOrder(byteOrder);
    byteStream << regValue;

    auto regBytes = RegisterBytes{};
    if (byteOrder == QDataStream::LittleEndian) {
        regBytes.low = static_cast<quint8>(bytes[0]);
        regBytes.high = static_cast<quint8>(bytes[1]);
    } else if (byteOrder == QDataStream::BigEndian) {
        regBytes.low = static_cast<quint8>(bytes[1]);
        regBytes.high = static_cast<quint8>(bytes[0]);
    }
    return regBytes;
}

quint16 ModbusDevicesGate::bytesToWord(const RegisterBytes &regBytes) const
{
    auto word = static_cast<quint16>(regBytes.low | regBytes.high << 8);
    return word;
}

QByteArray ModbusDevicesGate::packRegisterData(const QString &deviceName, const QModbusDataUnit::RegisterType tableType, const RegisterData &regData) const
{
    auto bytes = QByteArray{};
    QDataStream byteStream{ &bytes, QIODevice::WriteOnly };
    if (regData.isDword) {
        auto isHighWord = regData.highWordAddress == 0u;
        auto deviceRegisters = m_devicesDataMap.value(deviceName).value(tableType);
        auto secondWordBytes = isHighWord ? deviceRegisters.value(regData.lowWordAddress).bytes
                                          : deviceRegisters.value(regData.highWordAddress).bytes;
        auto highWordBytes = isHighWord ? regData.bytes : secondWordBytes;
        auto lowWordBytes = isHighWord ? secondWordBytes : regData.bytes;

        byteStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        byteStream << bytesToWord(highWordBytes) << bytesToWord(lowWordBytes);
    } else {
        byteStream << bytesToWord(regData.bytes);
    }
    return bytes;
}

QVariant ModbusDevicesGate::extractRegValueByType(const QMetaType::Type &type, const QByteArray &dataBytes, const QDataStream::ByteOrder byteOrder) const
{
    auto regValue = QVariant{};
    auto bytes = dataBytes;
    QDataStream byteStream{ &bytes, QIODevice::ReadOnly };
    byteStream.setByteOrder(byteOrder);
    switch (type) {
    case QMetaType::UShort:
        quint16 wordValue;
        byteStream >> wordValue;
        regValue = wordValue;
        break;
    case QMetaType::UInt:
        quint32 dwordValue;
        byteStream >> dwordValue;
        regValue = dwordValue;
        break;
    case QMetaType::Float:
    {
        float floatValue;
        byteStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        byteStream >> floatValue;
        regValue = floatValue;
        break;
    }
    default:
        mbDebug() << Q_FUNC_INFO << "ERROR: Unspecified register type.";
        break;
    }
    return regValue;
}

QList<quint16> ModbusDevicesGate::splitRegValue(const QVariant &regValue, const QMetaType::Type type) const
{
    auto bytes = QByteArray{};
    QDataStream writeStream{ &bytes, QIODevice::WriteOnly };
    switch (type) {
    case QMetaType::UShort: {
        quint16 wordValue = static_cast<quint16>(regValue.toUInt());
        writeStream << wordValue;
        break;
    }
    case QMetaType::UInt: {
        quint32 dwordValue = regValue.toUInt();
        writeStream << dwordValue;
        break;
    }
    case QMetaType::Float: {
        float floatValue = regValue.toFloat();
        writeStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        writeStream << floatValue;
        break;
    }
    default:
        mbDebug() << Q_FUNC_INFO << "ERROR: Unspecified register type.";
        break;
    }

    auto registersValueList = QList<quint16>();
    QDataStream readStream{ &bytes, QIODevice::ReadOnly };
    while (!readStream.atEnd()) {
        quint16 registerValue;
        readStream >> registerValue;
        registersValueList.append(registerValue);
    }
    return registersValueList;
}

ModbusRegistersTableMap ModbusDevicesGate::createRegistersMap(const QString &deviceName, const QModbusDataUnit::RegisterType tableType, const quint16 regAddress, const QVariant &regValue, const RegisterData &regInfo) const
{
    auto regMap = ModbusRegistersTableMap{};
    auto wordValueList = splitRegValue(regValue,
                                       getTypeByRegisterData(deviceName, tableType, regInfo));
    if (regInfo.isDword) {
        auto highWordAddress = hasHighAddress(regInfo) ? regInfo.highWordAddress
                                                       : getSecondWordAddress(regInfo);
        auto lowWordAddress = hasHighAddress(regInfo) ? getSecondWordAddress(regInfo)
                                                      : regInfo.lowWordAddress;
        regMap[tableType][highWordAddress] = wordValueList.first();
        regMap[tableType][lowWordAddress] = wordValueList.last();
    } else {
        regMap[tableType][regAddress] = wordValueList.first();
    }
    return regMap;
}

QString ModbusDevicesGate::getFirstDeviceName(const quint8 deviceId, const QModbusDataUnit::RegisterType tableType, const quint16 regAddress) const
{
    auto deviceAddressMap = m_deviceRegNames.value(deviceId);
    auto deviceAddressTable = AddressNamesTable{};
    if (!deviceAddressMap.isEmpty()) {
        deviceAddressTable = deviceAddressMap.value(tableType);
    }
    auto deviceName = QString{};
    if (!deviceAddressTable.isEmpty()) {
        deviceName = deviceAddressTable.value(regAddress).deviceName;
    }
    return deviceName;
}

QStringList ModbusDevicesGate::getDevicesNameList(const quint8 deviceId, const QModbusDataUnit::RegisterType tableType, const QList<quint16> &regAddresses) const
{
    if (regAddresses.isEmpty()) {
        return QStringList{};
    }

    auto namesList = QStringList{};
    for (auto regAddress : regAddresses) {
        auto deviceName = getFirstDeviceName(deviceId, tableType, regAddress);
        if (!deviceName.isEmpty() && !namesList.contains(deviceName)) {
            namesList.append(deviceName);
        }
    }
    return namesList;
}

Modbus::ModbusRegistersTableMap ModbusDevicesGate::buildRegisterDataByAddress(const QString &deviceName, const QModbusDataUnit::RegisterType tableType, const quint16 regAddress, const QVariant &regValue) const
{
    auto registerData = Modbus::ModbusRegistersTableMap{};
    auto deviceRegisters = m_devicesDataMap.value(deviceName).value(tableType);
    if (deviceRegisters.contains(regAddress)) {
        auto registerDataInfo = deviceRegisters.value(regAddress);
        registerData = createRegistersMap(deviceName, tableType, regAddress, regValue, registerDataInfo);
    }
    return registerData;
}

Modbus::ModbusRegistersTableMap ModbusDevicesGate::buildRegisterDataByInfoList(const QString &deviceName, const QList<RegisterInfo> &registersInfo, const QVariant &regValue) const
{
    auto registersMap = Modbus::ModbusRegistersTableMap{};
    auto sortedRegInfoList = registersInfo;
    sortRegistersInfo(sortedRegInfoList);
    bool valueIsList = regValue.canConvert<QVariantList>();
    auto regValueList = regValue.toList();
    for (quint8 regIndex = 0u; regIndex < sortedRegInfoList.count(); regIndex++) {
        auto regInfo = sortedRegInfoList.at(regIndex);
        auto newRegValue = valueIsList ? regValueList.at(regIndex) : regValue;
        auto registerData = buildRegisterDataByAddress(deviceName, regInfo.table, regInfo.index, newRegValue);
        if (!registerData.isEmpty()) {
            registersMap.insert(registerData);
        }
    }
    return registersMap;
}

Modbus::ModbusRegistersTableMap ModbusDevicesGate::jsonUnitToRegistersMap(const QVariantMap &jsonUnit) const
{
    auto deviceName = getFirstDeviceName(jsonUnit);
    if (!m_devicesConfigInfo.contains(deviceName)) {
        return ModbusRegistersTableMap{};
    }

    DeviceRegistersInfo registersInfo = m_devicesConfigInfo.value(deviceName);
    QVariantMap arrivedDeviceData = jsonUnit.first().toMap();
    auto registersMap = ModbusRegistersTableMap{};
    for (auto changedRegData = arrivedDeviceData.begin();
         changedRegData != arrivedDeviceData.end();
         changedRegData++)
    {
        auto regName = changedRegData.key();
        if (registersInfo.contains(regName)) {
            auto regInfoList = registersInfo.values(regName);
            auto registerData = buildRegisterDataByInfoList(deviceName, regInfoList, changedRegData.value());
            if (!registerData.isEmpty()) {
                registersMap.insert(registerData);
            }
        }
    }
    return registersMap;
}

bool ModbusDevicesGate::isPackedJsonUnit(const QVariantMap &jsonUnit) const
{
    auto depthLevel = getFirstDeviceDepthLevel(jsonUnit);
    bool isPacked = depthLevel > 1u;
    return isPacked;
}

QMetaType::Type ModbusDevicesGate::getTypeByRegisterData(const QString &deviceName, const QModbusDataUnit::RegisterType tableType, const RegisterData &regData) const
{
    if (!regData.isDword)
        return QMetaType::UShort;

    auto regAddress = hasHighAddress(regData) ? regData.highWordAddress
                                              : regData.lowWordAddress;
    auto deviceAddressTypes = m_deviceRegTypes.value(deviceName).value(tableType);
    if (!deviceAddressTypes.contains(regAddress)) {
        return QMetaType::Type{};
    }

    auto regType = deviceAddressTypes.value(regAddress);
    if (!QMetaType(regType).isValid()) {
        auto secondWordAddress = getSecondWordAddress(regData);
        regType = deviceAddressTypes.value(secondWordAddress);
    }
    return regType;
}

quint16 ModbusDevicesGate::calculateHighWordAddress(const quint16 lowWordAddress, const QDataStream::ByteOrder wordOrder) const
{
    auto highWordAddress = lowWordAddress;
    if (wordOrder == QDataStream::LittleEndian) {
        highWordAddress++;
    } else if (wordOrder == QDataStream::BigEndian) {
        highWordAddress--;
    }
    return highWordAddress;
}

quint16 ModbusDevicesGate::calculateLowWordAddress(const quint16 highWordAddress, const QDataStream::ByteOrder wordOrder) const
{
    auto lowWordAddress = highWordAddress;
    if (wordOrder == QDataStream::LittleEndian) {
        lowWordAddress--;
    } else if (wordOrder == QDataStream::BigEndian) {
        lowWordAddress++;
    }
    return lowWordAddress;
}

bool ModbusDevicesGate::hasHighAddress(const RegisterData &regData) const
{
    return regData.highWordAddress != 0u;
}

quint16 ModbusDevicesGate::getSecondWordAddress(const RegisterData &regInfo) const
{
    auto wordOrder = regInfo.endianess.word_order;
    auto secondWordAddress = hasHighAddress(regInfo) ? calculateLowWordAddress(regInfo.highWordAddress, wordOrder)
                                                     : calculateHighWordAddress(regInfo.lowWordAddress, wordOrder);
    return secondWordAddress;
}

QVariant ModbusDevicesGate::getRegValuesByName(const FullRegisterName &name) const
{
    // check if register is part of an array
    auto deviceRegisters = m_devicesConfigInfo.value(name.deviceName);
    auto regInfoList = deviceRegisters.values(name.registerName);
    bool canBeArray = regInfoList.count() > 1;
    bool allValuesValid = areValuesValid(regInfoList);

    auto dataValue = QVariant{};
    if (canBeArray && allValuesValid) {
        dataValue = buildRegValuesList(name.deviceName, regInfoList);
    } else if (!regInfoList.isEmpty()){
        dataValue = buildRegValue(name.deviceName, regInfoList.first());
    }
    return dataValue;
}

ModbusDevicesGate::AddressNamesTableMap ModbusDevicesGate::getDeviceAddressMap(const QString &deviceName) const
{
    auto deviceId = m_deviceIdNames.value(deviceName);
    auto deviceAddressMap = m_deviceRegNames.value(deviceId);
    return deviceAddressMap;
}

void ModbusDevicesGate::sortRegistersInfo(QList<Settings::RegisterInfo> &regInfoList) const
{
    std::sort(regInfoList.begin(), regInfoList.end(),
              [](const RegisterInfo &first, const RegisterInfo &second)
    {
        return first.index < second.index;
    });
}

void ModbusDevicesGate::setDeviceRegistersData(const QString &deviceName, const QModbusDataUnit::RegisterType tableType, const quint16 regAddress, const quint16 regValue)
{
    RegistersTable &deviceRegisters = m_devicesDataMap[deviceName][tableType];
    if (!deviceRegisters.contains(regAddress))
        return;

    RegisterData &currentReg = deviceRegisters[regAddress];
    auto currentRegValue = bytesToWord(currentReg.bytes);
    if (currentRegValue != regValue) {
        auto bytes = wordToBytes(regValue, currentReg.endianess.byte_order);
        currentReg.bytes = bytes;
    }
}

void ModbusDevicesGate::enqueueWriteRequest(const QVariantMap &requestData, const quint8 deviceId, const quint16 startAddress)
{
    auto request = WriteRequestInfo { requestData, deviceId, startAddress };
    m_writeRequests.append(request);
}

ModbusDevicesGate::WriteRequestInfo ModbusDevicesGate::dequeueWriteRequest(const QStringList &deviceNames, const quint8 deviceId, const quint16 startAddress)
{
    for (quint16 reqIndex = 0u; reqIndex < m_writeRequests.size(); reqIndex++) {
        auto request = m_writeRequests.at(reqIndex);
        if ((request.deviceId == deviceId)
                && (request.startAddress == startAddress)
                && deviceNames.contains(request.rawRequestData.firstKey()))
        {
            return m_writeRequests.takeAt(reqIndex);
        }
    }
    return WriteRequestInfo{};
}

ModbusDevicesGate::FullRegisterName ModbusDevicesGate::splitFullRegisterName(const QString &fullRegisterName) const
{
    if (fullRegisterName.isEmpty()) {
        return FullRegisterName{};
    }

    auto registerNameParts = fullRegisterName.split(".");
    if (registerNameParts.count() <= 1) {
        return FullRegisterName{};
    }

    auto splittedName = FullRegisterName{};
    splittedName.registerName = registerNameParts.takeLast(); // может содержать индекс массива уставок
    bool isIndex = false;
    splittedName.registerName.toUInt(&isIndex);
    if (isIndex) { // если является индексом, добавляем название массива
        splittedName.registerName = QString("%1.%2").arg(registerNameParts.takeLast(), splittedName.registerName);
    }
    splittedName.deviceName = registerNameParts.join(".");
    return splittedName;
}
