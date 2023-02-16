#include "modbusparsing.h"
#include "templates/algorithms.hpp"
#include "utils/wordoperations.h"

using namespace Utils;

bool mergeDataUnit(QList<QModbusDataUnit> &target, const QModbusDataUnit &unit) {
    for (auto &item : target) {
        if (item.startAddress() + item.valueCount() == unit.startAddress() &&
                item.registerType() == unit.registerType()) {
            item.setValues(item.values() + unit.values());
            return true;
        }
    }
    return false;
};

void sortAndMergeDataUnits(QList<QModbusDataUnit> &target) {
    auto sorter = [](const QModbusDataUnit &lh, const QModbusDataUnit &rh) {
        return lh.startAddress() < rh.startAddress();
    };
    std::sort(target.begin(), target.end(), sorter);
    auto copy = target;
    QList<int> toRemove{};
    for (auto unit : Radapter::enumerate(&copy)) {
        if (mergeDataUnit(target, unit.value)) {
            toRemove.append(unit.count);
        }
    }
    for (auto index : Radapter::enumerate(&toRemove)) {
        target.removeAt(index.value - index.count);
    }
};
QList<QModbusDataUnit> Modbus::mergeDataUnits(const QList<QModbusDataUnit> &src)
{
    QList<QModbusDataUnit> holdingResult;
    QList<QModbusDataUnit> diResult;
    QList<QModbusDataUnit> inputResult;
    QList<QModbusDataUnit> coilsResult;
    for (auto &unit : src) {
        switch (unit.registerType()) {
        case QModbusDataUnit::HoldingRegisters:
            if (!mergeDataUnit(holdingResult, unit)) {
                holdingResult.append(unit);
            }
            continue;
        case QModbusDataUnit::DiscreteInputs:
            if (!mergeDataUnit(diResult, unit)) {
                diResult.append(unit);
            }
            continue;
        case QModbusDataUnit::Coils:
            if (!mergeDataUnit(coilsResult, unit)) {
                coilsResult.append(unit);
            }
            continue;
        case QModbusDataUnit::InputRegisters:
            if (!mergeDataUnit(inputResult, unit)) {
                inputResult.append(unit);
            }
            continue;
        default:
            continue;
        }
    }
    sortAndMergeDataUnits(holdingResult);
    sortAndMergeDataUnits(diResult);
    sortAndMergeDataUnits(inputResult);
    sortAndMergeDataUnits(coilsResult);
    return holdingResult += diResult += inputResult += coilsResult;
}

QModbusDataUnit Modbus::parseValueToDataUnit(const QVariant &src, const Settings::RegisterInfo &regInfo, const Settings::PackingMode &endianess)
{
    if (!src.canConvert(regInfo.type)) {
        reError() << "Error writing data to modbus: "
                  << src << "; Index: " << regInfo.index;
        return {};
    }
    const auto sizeWords = QMetaType::sizeOf(regInfo.type)/2;
    auto copy = src;
    auto words = toWords(copy.data(), sizeWords);
    applyEndianness(words.data(), endianess, sizeWords, false);
    return QModbusDataUnit{regInfo.table, regInfo.index, words};
}

QVariant Modbus::parseModbusType(quint16 *words, const Settings::RegisterInfo &regInfo, int sizeWords, const Settings::PackingMode &endianess)
{
    applyEndianness(words, endianess, sizeWords, true);
    switch(regInfo.type) {
    case QMetaType::UShort:
        return bit_cast<quint16>(words);
    case QMetaType::UInt:
        return bit_cast<quint32>(words);
    case QMetaType::Float:
        return bit_cast<float>(words);
    default:
        return{};
    }
}

QString Modbus::tableToString(QModbusDataUnit::RegisterType type)
{
    switch (type) {
    case QModbusDataUnit::RegisterType::Invalid: return "Invalid";
    case QModbusDataUnit::RegisterType::Coils: return "Coils";
    case QModbusDataUnit::RegisterType::HoldingRegisters: return "HoldingRegisters";
    case QModbusDataUnit::RegisterType::InputRegisters: return "InputRegisters";
    case QModbusDataUnit::RegisterType::DiscreteInputs: return "DiscreteInputs";
    default: return "Unknown";
    }
}

QString Modbus::printUnit(const QModbusDataUnit &unit)
{
    auto result = "Table: " + tableToString(unit.registerType()) + "; Vals: [";
    for (auto &val : unit.values()) {
        result.append(QString::number(val, 16)).append(", ");
    }
    return  result + "]; --> Start: " + QString::number(unit.startAddress()) + "; Count: " + QString::number(unit.valueCount());
}