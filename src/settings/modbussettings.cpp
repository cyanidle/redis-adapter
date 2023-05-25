#include "modbussettings.h"
#include "jsondict/jsondict.h"
#include "modbus/modbusparsing.h"
#include "sync/channel.h"
#include <QThread>
#include <QStringBuilder>

using namespace Settings;
using namespace Serializable;
typedef QMap<QString /*regName*/, RegisterInfo> DevicesRegisters;
typedef QMap<QString /*deviceName*/, DevicesRegisters> AllRegisters;
Q_GLOBAL_STATIC(AllRegisters, allRegisters)
Q_GLOBAL_STATIC(QStringMap<ModbusDevice>, devicesMap)

void ModbusSlave::postUpdate() {
    for (auto &name: register_names) {
        auto toMerge = (*allRegisters).value(name.replace('.', ':'));
        for (auto newRegisters = toMerge.cbegin(); newRegisters != toMerge.cend(); ++newRegisters) {
            auto name = newRegisters.key();
            auto &reg = newRegisters.value();
            if (registers.contains(name)) {
                throw std::invalid_argument("Register name collision: " + name.toStdString());
            }
            registers[name] = reg;
        }
    }
    auto errRange = [](const RegisterInfo &info){
        throw std::runtime_error(QString(
            "Register ("%Modbus::printTable(info.table)%'['%QString::number(info.index)%"]) "
            "out of range for slave: all regs must be from 0 to n without gaps").toStdString());
    };
    counts.reset();
    for (auto &reg : registers) {
        auto wordSize = QMetaType(reg.type).sizeOf() / 2;
        switch (reg.table.value) {
        case QModbusDataUnit::InputRegisters:
            if (reg.index > counts.input_registers) {
                errRange(reg);
            }
            counts.input_registers+=wordSize;
            continue;
        case QModbusDataUnit::Coils:
            if (reg.index > counts.coils) {
                errRange(reg);
            }
            counts.coils+=wordSize;
            continue;
        case QModbusDataUnit::HoldingRegisters:
            if (reg.index > counts.holding_registers) {
                errRange(reg);
            }
            counts.holding_registers+=wordSize;
            continue;
        case QModbusDataUnit::DiscreteInputs:
            if (reg.index > counts.di) {
                errRange(reg);
            }
            counts.di+=wordSize;
            continue;
        default:
            throw std::runtime_error("Unkown registers error");
        }
    }
    if (registers.isEmpty()) {
        throw std::runtime_error("Empty registers for Mb Slave! Available: " + allRegisters->keys().join(", ").toStdString());
    }
    device = devicesMap->value(device_name);
}

void ModbusMaster::postUpdate()
{
    for (auto &name: register_names) {
        auto toMerge = (*allRegisters).value(name.replace('.', ':'));
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
    for (auto &query: queries) {
        auto &table = query.type;
        auto start = query.reg_index;
        auto end = query.reg_count + start;
        auto hitCount = 0;
        for (auto &reg: registers) {
            if (reg.table == table && reg.index >= start && reg.index < end) {
                hitCount++;
            }
        }
        if (hitCount < query.reg_count || hitCount > query.reg_count) {
            throw std::runtime_error(QString("Query: reg_index: "%
                                             QString::number(query.reg_index)%
                                             ". reg_count: "%
                                             QString::number(query.reg_count)%
                                             ". table: "%
                                             Modbus::printTable(query.type)%
                                             ": not all registers found!").toStdString());
        }
    }
    device = devicesMap->value(device_name);
}

const QString &Validator::ByteWordOrder::name()
{
    static QString stName = "ByteOrder: abcd/badc/cdab/dcba";
    return stName;
}

bool Validator::ByteWordOrder::validate(QVariant &value)
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

const QString &Validator::RegValueType::name()
{
    static QString stName = "Register Value Type: uint16/word/uint32/dword/float/float32";
    return stName;
}

bool Validator::RegValueType::validate(QVariant &value) {
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

const QString &Validator::RegisterTable::name()
{
    static QString stName = "Register Table Type: holding/input/coils/discrete_inputs/holding_registers/input_registers/coils/di/do";
    return stName;
}

bool Validator::RegisterTable::validate(QVariant &value) {
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

PackingMode::PackingMode(QDataStream::ByteOrder words, QDataStream::ByteOrder bytes) :
    words(words), bytes(bytes)
{}


void ModbusDevice::postUpdate() {
    static QThread channelsThread;
    if (!channel) channel.reset(new Radapter::Sync::Channel(&channelsThread));
    channelsThread.start();
    if (tcp.isValid() && rtu.isValid()) {
        throw std::runtime_error("[Modbus Device] Both tcp and rtu device is prohibited! Use one");
    } else if (!tcp.isValid() && !rtu.isValid()) {
        throw std::runtime_error("[Modbus Device] Tcp or Rtu device not specified!");
    }
    devicesMap->insert(tcp->port ? tcp->name : rtu->name, *this);
}

void Registers::init(const QString &device)
{
    auto parseRegisters = [&](const QVariantMap &regs, const QVariant &toInsert) {
        const auto deviceRegs = JsonDict(regs);
        for (auto &iter : deviceRegs) {
            if (iter.field() == "index" && iter.value().canConvert<int>()) {
                auto regName = iter.domainKey().join(":");
                auto regInfo = *iter.domainMap();
                regInfo["table"] = toInsert;
                (*allRegisters)[device][regName] = parseObject<RegisterInfo>(regInfo);
            }
        }
    };
    parseRegisters(holding, "holding");
    parseRegisters(holding_registers, "holding");
    parseRegisters(input, "input");
    parseRegisters(input_registers, "input");
    parseRegisters(coils, "coils");
    parseRegisters(di, "discrete_inputs");
    parseRegisters(discrete_inputs, "discrete_inputs");
}

void RegisterCounts::reset()
{
    coils = di = input_registers = holding_registers = 0;
}
