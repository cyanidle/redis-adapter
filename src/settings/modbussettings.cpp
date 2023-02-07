#include "modbussettings.h"
#include "templates/algorithms.hpp"

using namespace Settings;
using namespace Serializer;
Q_GLOBAL_STATIC(DevicesRegisters, allRegisters)

void Settings::parseRegisters(const QVariant &registersFile) {
    if (!allRegisters->isEmpty()) {
        throw std::runtime_error("Register parsing called second time!");
    }
    auto fileJson = JsonDict{registersFile.toMap()};
    for (auto &fileIter : fileJson) {
        if (fileIter.field() != "__table__") continue;
        auto deviceName = fileIter.domain().join(':').replace('.', ':');
        auto currentFlatJson = fileJson.value(fileIter.domain()).toMap();
        auto table = currentFlatJson.take("__table__");
        Registers subresult;
        for (auto iter{currentFlatJson.cbegin()}; iter != currentFlatJson.cend(); ++iter) {
            if (iter.value().canConvert<QVariantList>()) {
                auto currentRegs = iter.value().toList();
                for (auto reg : Radapter::enumerate(currentRegs)) {
                    auto currentReg = reg.value.toMap();
                    if (currentReg.contains("__table__")) continue;
                    if (!currentReg.first().toMap().isEmpty()) {
                        throw std::invalid_argument("Incorrect registers formatting");
                    }
                    currentReg.insert("table", table);
                    subresult.insert(deviceName + ':' + iter.key() + ":" + QString::number(reg.count + 1), fromQMap<RegisterInfo>(currentReg));
                }
            } else if (iter.value().canConvert<QVariantMap>()) {
                auto currentReg = iter.value().toMap();
                if (currentReg.contains("__table__")) continue;
                if (!currentReg.first().toMap().isEmpty()) {
                    throw std::invalid_argument("Incorrect registers formatting");
                }
                currentReg.insert("table", table);
                subresult.insert(deviceName + ':' + iter.key(), fromQMap<RegisterInfo>(currentReg));
            } else {
                throw std::invalid_argument("Incorrect registers formatting");
            }
        }
        allRegisters->insert(deviceName, subresult);
    }
}

void ModbusSlave::postInit() {
    for (auto &name: register_names) {
        registers = (*allRegisters)[name.replace('.', ':')];
    }
    for (auto &reg : registers) {
        switch (reg.table) {
        case QModbusDataUnit::InputRegisters:
            counts.input_registers++;
            continue;
        case QModbusDataUnit::Coils:
            counts.coils++;
            continue;
        case QModbusDataUnit::HoldingRegisters:
            counts.holding_registers++;
            continue;
        case QModbusDataUnit::DiscreteInputs:
            counts.di++;
            continue;
        default:
            throw std::runtime_error("Unkown registers error");
        }
    }
    if (registers.isEmpty()) {
        throw std::runtime_error("Empty registers for Mb Slave! Available: " + allRegisters->keys().join(", ").toStdString());
    }
}

void ModbusMaster::postInit()
{
    for (auto &name: register_names) {
        registers = (*allRegisters)[name.replace('.', ':')];
    }
    if (registers.isEmpty()) {
        throw std::runtime_error("Empty registers for Mb Master!");
    }
    device = Settings::ModbusDevice::get(device_name);
}
