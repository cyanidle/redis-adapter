#include "modbussettings.h"

using namespace Settings;

RegisterInfo::tableMap RegisterInfo::StringToTable = tableMap{
        {"holding",QModbusDataUnit::RegisterType::HoldingRegisters},
        {"input",QModbusDataUnit::RegisterType::InputRegisters},
        {"coils",QModbusDataUnit::RegisterType::Coils}
        };
RegisterInfo::typeMap RegisterInfo::StringToType = typeMap{
        {"uint16", QMetaType::UShort},
        {"uint32", QMetaType::UInt},
        {"float", QMetaType::Float}
        };
SerialDevice::Map SerialDevice::cacheMap = SerialDevice::Map{};
ModbusConnectionSource::Map ModbusConnectionSource::cacheMap = ModbusConnectionSource::Map{};

DeviceRegistersInfoMap DeviceRegistersInfoMapParser::parse(const QVariant &src) {
    auto result = DeviceRegistersInfoMap();
    auto json = Formatters::JsonDict(src.toMap());
    for (auto &iter: json) {
        auto fullKey = iter.getFullKey();
        if (fullKey.constLast() == "table") {
            auto tableType = iter.value().toString();
            auto currentRegs = DeviceRegistersInfo();
            auto keyToRegisters = iter.getCurrentDomain();
            auto domainJson = Formatters::JsonDict(json.value(keyToRegisters).toMap());
            auto byteorder = domainJson.value("byte_order").toString().toLower();
            auto wordorder = domainJson.value("word_order").toString().toLower();
            if (byteorder.isEmpty() || wordorder.isEmpty()) {
                reDebug() << "'byte_order' or 'word_order' not set in registers config!";
            }
            auto regInfoMapJson = Formatters::JsonDict(domainJson.value("registers").toMap());

            for (auto &regInfoIter: regInfoMapJson){
                if (regInfoIter.value().canConvert<QVariantList>()) {
                    auto currentRegisterMapsList = regInfoIter.value().toList();
                    for (int i = 0; i < currentRegisterMapsList.length(); ++i) {
                        auto currentRegisterMap = currentRegisterMapsList[i].toMap();
                        currentRegisterMap.insert("table", tableType);
                        currentRegisterMap.insert("endianess", QVariantMap{{"word_order", wordorder}, {"byte_order", byteorder}});
                        currentRegisterMap.insert("is_persistent", currentRegisterMap.value("is_persistent"));
                        auto currentRegister = Serializer::fromQMap<RegisterInfo>(currentRegisterMap);
                        currentRegs.insert(regInfoIter.getFullKey().join(":") + ":" + QString::number(i + 1), currentRegister);
                    }
                } else if (regInfoIter.field() == "index") {
                    auto currentRegisterMap = regInfoMapJson[regInfoIter.getCurrentDomain()].toMap();
                    currentRegisterMap.insert("table", tableType);
                    currentRegisterMap.insert("endianess", QVariantMap{{"word_order", wordorder},
                                                                       {"byte_order", byteorder}});
                    currentRegisterMap.insert("is_persistent", currentRegisterMap.value("is_persistent"));
                    auto currentRegister = Serializer::fromQMap<RegisterInfo>(currentRegisterMap);
                    currentRegs.insert(regInfoIter.getCurrentDomain().join(":"), currentRegister);
                }
            }
            if (!currentRegs.isEmpty()) {
                result.insert(keyToRegisters.join("."), currentRegs);
            }
        }
    }
    return result;
}
