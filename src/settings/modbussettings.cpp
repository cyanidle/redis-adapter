#include "modbussettings.h"
#include "jsondict/jsondict.hpp"
#include "templates/algorithms.hpp"

using namespace Settings;
using namespace Serializable;
Q_GLOBAL_STATIC(DevicesRegisters, allRegisters)

void Settings::parseRegisters(const QVariantMap &registersFile) {
    if (!allRegisters->isEmpty()) {
        throw std::runtime_error("Register parsing called second time!");
    }
    auto parseRegisters = [&](const QVariantMap &src, const QVariant &toInsert) {
        for (auto iter = src.begin(); iter != src.end(); ++iter) {
            auto deviceName = iter.key();
            const auto deviceRegs = JsonDict(iter.value());
            for (auto &iter : deviceRegs) {
                if (iter.field() == "index" && iter.value().canConvert<int>()) {
                    auto regName = iter.domainKey().join(":");
                    auto regInfo = *iter.domainMap();
                    regInfo["table"] = toInsert;
                    (*allRegisters)[deviceName][regName] = fromQMap<RegisterInfo>(regInfo);
                }
            }
        }
    };
    parseRegisters(registersFile["holding_registers"].toMap(), "holding");
    parseRegisters(registersFile["input_registers"].toMap(), "input");
    parseRegisters(registersFile["coils"].toMap(), "coils");
    parseRegisters(registersFile["discrete_inputs"].toMap(), "discrete_inputs");
}

void ModbusSlave::postUpdate() {
    for (auto &name: register_names) {
        auto toMerge = (*allRegisters).value(name.replace('.', ':'));
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
        auto wordSize = QMetaType(reg.type).sizeOf() / 2;
        switch (reg.table.value) {
        case QModbusDataUnit::InputRegisters:
            counts.input_registers+=wordSize;
            continue;
        case QModbusDataUnit::Coils:
            counts.coils+=wordSize;
            continue;
        case QModbusDataUnit::HoldingRegisters:
            counts.holding_registers+=wordSize;
            continue;
        case QModbusDataUnit::DiscreteInputs:
            counts.di+=wordSize;
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
        if (toMerge.isEmpty()) {
            throw std::runtime_error(worker->name->toStdString() + ": Missing registers with name: " + name.toStdString());
        }
        for (auto newRegisters = toMerge.begin(); newRegisters != toMerge.end(); ++newRegisters) {
            auto name = newRegisters.key();
            auto &reg = newRegisters.value();
            if (registers.contains(name)) {
                throw std::invalid_argument("Register name collision: " + name.toStdString());
            }
            registers[name] = reg;
        }
    }
    device = Settings::ModbusDevice::get(device_name);
}

bool OrdersValidator::validate(QVariant &value)
{
    auto asStr = value.toString().toLower();
    if (asStr.isEmpty()) return true;
    bool wordsBig = false;
    bool bytesBig = false;
    if (asStr == QStringLiteral("abcd")) {
        wordsBig = bytesBig = true;
    } else if (asStr == QStringLiteral("badc")) {
        bytesBig = true;
    } else if (asStr == QStringLiteral("cdab")) {
        wordsBig = true;
    } else if (asStr == QStringLiteral("dcba")) {
        wordsBig = bytesBig = false;
    } else {
        throw std::runtime_error("Invalid endianess format: " + asStr.toStdString() + "; Available: abcd | badc | cdab | dcba");
    }
    value = QVariantMap {
        {"bytes", bytesBig ? "big" : "little"},
        {"words", wordsBig ? "big" : "little"},
    };
    return true;
}




