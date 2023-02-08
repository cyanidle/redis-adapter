#include "wordoperations.h"
#include "templates/algorithms.hpp"

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

QList<QModbusDataUnit> Utils::mergeDataUnits(const QList<QModbusDataUnit> &src)
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

QModbusDataUnit Utils::parseValueToDataUnit(const QVariant &src, const Settings::RegisterInfo &regInfo, const Settings::PackingMode &endianess)
{
    if (!src.canConvert(regInfo.type)) {
        reError() << "Error writing data to modbus: "
                  << src << "; Index: " << regInfo.index;
        return {};
    }
    const auto sizeWords = QMetaType::sizeOf(regInfo.type)/2;
    auto copy = src;
    if (!copy.convert(regInfo.type)) {
        reWarn() << "MbSlave: Conversion error!";
        return {};
    }
    auto words = toWords(copy.data(), sizeWords);
    applyEndianness(words.data(), endianess, sizeWords, false);
    return QModbusDataUnit{regInfo.table, regInfo.index, words};
}

QVariant Utils::parseModbusType(quint16 *words, const Settings::RegisterInfo &regInfo, int sizeWords, const Settings::PackingMode &endianess)
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

void Utils::applyEndianness(quint16 *words, const Settings::PackingMode endianess, int sizeWords, bool receive) {
    auto differentEndian = getEndianess() != endianess.byte_order;
    if (!receive && differentEndian) {
        applyToBytes(words, sizeWords, endianess.byte_order);
    }
    if (sizeWords > 1) {
        applyToWords(words, sizeWords, endianess.word_order);
    }
    if (receive && differentEndian) {
        applyToBytes(words, sizeWords, endianess.byte_order);
    }
}

QVector<quint16> Utils::toWords(const void *src, int sizeWords) {
    QVector<quint16> result(sizeWords);
    std::memcpy(result.data(), src, sizeWords*2);
    return result;
}

void Utils::applyToBytes(quint16 *words, int sizeWords, QDataStream::ByteOrder byteOrder) {
    if (byteOrder != getEndianess()) {
        auto sizeBytes = sizeWords*2;
        QVector<quint8> bytes(sizeBytes);
        std::memcpy(bytes.data(), words, sizeBytes);
        for (int i = 0; i < sizeBytes/2; ++i) {
            std::swap(bytes[i], bytes[sizeBytes - 1 - i]);
        }
        std::memcpy(words, bytes.data(), sizeBytes);
    }
}

void Utils::applyToWords(quint16 *words, int sizeWords, QDataStream::ByteOrder wordOrder) {
    if (wordOrder != getEndianess()) {
        for (int i = 0; i < sizeWords/2; ++i) {
            std::swap(words[i], words[sizeWords - i - 1]);
        }
    }
}
