#include "modbussettings.h"
#include "templates/algorithms.hpp"

using namespace Settings;
using namespace Serializable;
Q_GLOBAL_STATIC(DevicesRegisters, allRegisters)

void Settings::parseRegisters(const QVariant &registersFile) {
    if (!allRegisters->isEmpty()) {
        throw std::runtime_error("Register parsing called second time!");
    }
    auto fileJson = JsonDict{registersFile.toMap()};
    for (auto &fileIter : fileJson) {
        if (fileIter.field() != "__table__") continue;
        auto deviceName = fileIter.domainKey().join(':').replace('.', ':');
        auto currentFlatJson = fileJson.value(fileIter.domainKey()).toMap();
        auto table = currentFlatJson.take("__table__");
        Registers currentDeviceResult;
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
                    currentDeviceResult.insert(deviceName + ':' + iter.key() + ":" + QString::number(reg.count + 1), fromQMap<RegisterInfo>(currentReg));
                }
            } else if (iter.value().canConvert<QVariantMap>()) {
                auto currentRegs = JsonDict{iter.value().toMap()};
                if (currentRegs.contains("__table__")) continue;
                if (currentRegs.depth()) {
                    for (auto &subiter : currentRegs) {
                        auto currentReg = currentRegs[subiter.domainKey()].toMap();
                        currentReg.insert("table", table);
                        currentDeviceResult.insert(deviceName + ':' + iter.key() + ":" + subiter.domainKey().join(":"), fromQMap<RegisterInfo>(currentReg));
                    }
                } else {
                    currentRegs.insert("table", table);
                    currentDeviceResult.insert(deviceName + ':' + iter.key(), fromQMap<RegisterInfo>(currentRegs));
                }
            } else {
                throw std::invalid_argument("Incorrect registers formatting");
            }
        }
        allRegisters->insert(deviceName, currentDeviceResult);
    }
}

void ModbusSlave::postUpdate() {
    for (auto &name: register_names) {
        auto &toMerge = (*allRegisters).value(name.replace('.', ':'));
        for (auto newRegisters = toMerge.begin(); newRegisters != toMerge.end(); ++newRegisters) {
            auto name = newRegisters.key();
            auto &reg = newRegisters.value();
            if (registers.contains(name)) {
                throw std::invalid_argument("Register name collision: " + name.toStdString());
            }
            registers[name] = reg;
        }
    }
    for (auto &reg : registers) {
        switch (reg.table.value) {
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
    device = Settings::ModbusDevice::get(device_name);
}

void ModbusMaster::postUpdate()
{
    for (auto &name: register_names) {
        auto &toMerge = (*allRegisters)[name.replace('.', ':')];
        for (auto newRegisters = toMerge.begin(); newRegisters != toMerge.end(); ++newRegisters) {
            auto name = newRegisters.key();
            auto &reg = newRegisters.value();
            if (registers.contains(name)) {
                throw std::invalid_argument("Register name collision: " + name.toStdString());
            }
            registers[name] = reg;
        }
    }
    if (registers.isEmpty()) {
        throw std::runtime_error("Empty registers for Mb Master!");
    }
    device = Settings::ModbusDevice::get(device_name);
}
