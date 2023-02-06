#include "modbussettings.h"

using namespace Settings;;
SerialDevice::Map SerialDevice::cacheMap = SerialDevice::Map{};
ModbusConnectionSource::Map ModbusConnectionSource::cacheMap = ModbusConnectionSource::Map{};

DeviceRegistersInfoMap DeviceRegistersInfoMapParser::parse(const QVariant &src) {
    auto result = DeviceRegistersInfoMap();
    auto json = JsonDict(src.toMap());
    for (auto &iter: json) {
        auto fullKey = iter.key();
        if (fullKey.constLast() == "table") {
            auto tableType = iter.value().toString();
            auto currentRegs = DeviceRegistersInfo();
            auto keyToRegisters = iter.domain();
            auto domainJson = JsonDict(json.value(keyToRegisters).toMap());
            auto byteorder = domainJson.value("byte_order").toString().toLower();
            auto wordorder = domainJson.value("word_order").toString().toLower();
            if (byteorder.isEmpty() || wordorder.isEmpty()) {
                throw std::runtime_error("'byte_order' or 'word_order' not set in registers config!");
            }
            auto regInfoMapJson = JsonDict(domainJson.value("registers").toMap());
            for (auto &regInfoIter: regInfoMapJson){
                if (regInfoIter.value().canConvert<QVariantList>()) {
                    auto currentRegisterMapsList = regInfoIter.value().toList();
                    for (int i = 0; i < currentRegisterMapsList.length(); ++i) {
                        auto currentRegisterMap = currentRegisterMapsList[i].toMap();
                        currentRegisterMap.insert("table", tableType);
                        currentRegisterMap.insert("endianess", QVariantMap{{"word_order", wordorder}, {"byte_order", byteorder}});
                        currentRegisterMap.insert("is_persistent", currentRegisterMap.value("is_persistent"));
                        auto currentRegister = Serializer::fromQMap<RegisterInfo>(currentRegisterMap);
                        currentRegs.insert(regInfoIter.key().join(".") + "." + QString::number(i + 1), currentRegister);
                    }
                } else if (regInfoIter.field() == "index") {
                    auto currentRegisterMap = regInfoMapJson[regInfoIter.domain()].toMap();
                    currentRegisterMap.insert("table", tableType);
                    currentRegisterMap.insert("endianess", QVariantMap{{"word_order", wordorder},
                                                                       {"byte_order", byteorder}});
                    currentRegisterMap.insert("is_persistent", currentRegisterMap.value("is_persistent"));
                    auto currentRegister = Serializer::fromQMap<RegisterInfo>(currentRegisterMap);
                    currentRegs.insert(regInfoIter.domain().join("."), currentRegister);
                }
            }
            if (!currentRegs.isEmpty()) {
                result.insert(keyToRegisters.join("."), currentRegs);
            }
        }
    }
    return result;
}

bool ModbusSlaveWorker::parseHolding(const QVariant &src) {
    auto map = src.toMap();
    map.insert("table_type", "holding");
    auto regs = parseRegisters(map);
    registers.unite(regs);
    holding_registers = regs.count();
    return true;
}
bool ModbusSlaveWorker::parseInput(const QVariant &src) {
    auto map = src.toMap();
    map.insert("table_type", "input");
    auto regs = parseRegisters(map);
    registers.unite(regs);
    input_registers = regs.count();
    return true;
}
bool ModbusSlaveWorker::parseDi(const QVariant &src) {
    auto map = src.toMap();
    map.insert("table_type", "discrete_inputs");
    auto regs = parseRegisters(map);
    registers.unite(regs);
    di = regs.count();
    return true;
}
bool ModbusSlaveWorker::parseCoils(const QVariant &src) {
    auto map = src.toMap();
    map.insert("table_type", "coils");
    auto regs = parseRegisters(map);
    registers.unite(regs);
    coils = regs.count();
    return true;
}
DeviceRegistersInfo ModbusSlaveWorker::parseRegisters(const JsonDict &target) {
    DeviceRegistersInfo result;
    for (auto &iter : target) {
        if (iter.field() == "index") {
            auto domain = iter.domain();
            auto map = target[domain].toMap();
            map.insert("table", target["table_type"]);
            auto reg = Serializer::fromQMap<Settings::RegisterInfo>(map);
            result.insert(domain.join(":"), reg);
        }
    }
    return result;
}

void ModbusSlaveWorker::postInit() const {
    if (registers.isEmpty()) {
        throw std::runtime_error("Empty registers for MbSlave worker!");
    }
    for (const auto &reg : registers) {
        switch (reg.table) {
        case QModbusDataUnit::Coils:
            if (reg.index > coils) throw std::runtime_error("Registers must be from 0 with step 1");
            break;
        case QModbusDataUnit::HoldingRegisters:
            if (reg.index > holding_registers) throw std::runtime_error("Registers must be from 0 with step 1");
            break;
        case QModbusDataUnit::InputRegisters:
            if (reg.index > input_registers) throw std::runtime_error("Registers must be from 0 with step 1");
            break;
        case QModbusDataUnit::DiscreteInputs:
            if (reg.index > di) throw std::runtime_error("Registers must be from 0 with step 1");
            break;
        default:
            throw std::runtime_error("Registers error");
        }
    }
}


