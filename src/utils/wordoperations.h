#ifndef WORDOPERATIONS_H
#define WORDOPERATIONS_H

#include "settings/modbussettings.h"
#include <cstring>
#include <QDataStream>

namespace Utils {

template<typename T>
inline QVector<quint16> toWords(const T& src) {
    static_assert(!(sizeof(T)%2), "Cannot cast to words type with odd length");
    quint16 result[sizeof(T)/2];
    std::memcpy(result, &src, sizeof(T));
    auto out = QVector<quint16>(sizeof(T)/2);
    for (int i = 0; i < sizeof(T)/2; ++i) {
        out[i] = result[i];
    }
    return out;
}

constexpr QDataStream::ByteOrder getEndianess() {
    return Q_BYTE_ORDER == Q_LITTLE_ENDIAN ? QDataStream::LittleEndian : QDataStream::BigEndian;
}

template <typename To, typename From>
typename std::enable_if<std::is_trivially_copyable<To>::value &&
                        std::is_trivially_copyable<From>::value &&
                        !std::is_pointer<From>::value,
To>::type
bit_cast(const From* src) {
    To result;
    std::memcpy(&result, src, sizeof(To));
    return result;
}

template <typename To, typename From>
typename std::enable_if<sizeof(To) == sizeof(From) &&
                        std::is_trivially_copyable<To>::value &&
                        std::is_trivially_copyable<From>::value &&
                        !std::is_pointer<From>::value,
To>::type
bit_cast(const From& src) {
    To result;
    std::memcpy(&result, &src, sizeof(To));
    return result;
}

QVariant parseModbusType(quint16* words, const Settings::RegisterInfo &regInfo, int sizeWords, const Settings::PackingMode &endianess);
QModbusDataUnit parseValueToDataUnit(const QVariant &src, const Settings::RegisterInfo &regInfo, const Settings::PackingMode &endianess);
QList<QModbusDataUnit> mergeDataUnits(const QList<QModbusDataUnit> &src);
void applyEndianness(quint16 *words, const Settings::PackingMode endianess, int sizeWords, bool receive);
QVector<quint16> toWords(const void* src, int sizeWords);
void applyToWords(quint16 *words, int sizeWords, QDataStream::ByteOrder wordOrder);
void applyToBytes(quint16 *words, int sizeWords, QDataStream::ByteOrder byteOrder);


}

#endif // WORDOPERATIONS_H
