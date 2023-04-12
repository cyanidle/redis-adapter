#include "modbussettings.h"
#include "jsondict/jsondict.h"
#include "templates/algorithms.hpp"

using namespace Settings;
using namespace Serializable;
typedef QMap<QString /*deviceName*/, Registers> DevicesRegisters;
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
                    (*allRegisters)[deviceName][regName] = parseObject<RegisterInfo>(regInfo);
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
        auto wordSize = QMetaType::sizeOf(reg.type) / 2;
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
        auto &toMerge = (*allRegisters).value(name.replace('.', ':'));
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

void RegisterInfo::postUpdate()
{
    if (table == QModbusDataUnit::InputRegisters || table == QModbusDataUnit::DiscreteInputs) {
        writable = false;
    }
}

bool ChooseRegValueType::validate(QVariant &value) {
    static QMap<QString, QMetaType::Type>
        map{{"uint16", QMetaType::UShort},
            {"word", QMetaType::UShort},
            {"uint32", QMetaType::UInt},
            {"dword", QMetaType::UInt},
            {"float", QMetaType::Float},
            {"float32", QMetaType::Float}};
    auto asStr = value.toString().toLower();
    value.setValue(map.value(asStr));
    return map.contains(asStr);
}

bool ChooseRegisterTable::validate(QVariant &value) {
    static QMap<QString, QModbusDataUnit::RegisterType>
        map{{"holding",QModbusDataUnit::HoldingRegisters},
            {"input",QModbusDataUnit::InputRegisters},
            {"coils",QModbusDataUnit::Coils},
            {"discrete_inputs",QModbusDataUnit::DiscreteInputs},
            {"holding_registers",QModbusDataUnit::HoldingRegisters},
            {"input_registers",QModbusDataUnit::InputRegisters},
            {"coils",QModbusDataUnit::Coils},
            {"di",QModbusDataUnit::DiscreteInputs},
            {"do",QModbusDataUnit::Coils},};
    auto asStr = value.toString().toLower();
    value.setValue(map.value(asStr));
    return map.contains(asStr);
}
