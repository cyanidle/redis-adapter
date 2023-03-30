#ifndef WORDOPERATIONS_H
#define WORDOPERATIONS_H

#include "settings/modbussettings.h"
#include <cstring>
#include <QDataStream>

namespace ByteUtils {

constexpr QDataStream::ByteOrder getEndianess() {
    return Q_BYTE_ORDER == Q_LITTLE_ENDIAN ? QDataStream::LittleEndian : QDataStream::BigEndian;
}

template <typename To, typename From>
typename std::enable_if<std::is_trivially_copyable<To>::value &&
                        std::is_trivially_copyable<From>::value,
To>::type
bit_cast(const From* src) {
    To result;
    std::memcpy(&result, src, sizeof(To));
    return result;
}

void applyEndianness(quint16 *words,
                     const Settings::PackingMode endianess,
                     int sizeWords,
                     bool receive);
void applyToWords(quint16 *words, int sizeWords, QDataStream::ByteOrder wordOrder);
void applyToBytes(quint16 *words, int sizeWords, QDataStream::ByteOrder byteOrder);

QVector<quint16> toWords(const void* src, int sizeWords);
QByteArray toBytes(const void* src, int sizeBytes);
template<typename T>
inline QVector<quint16> toWords(const T& src) {
    static_assert(!(sizeof(T)%2), "Cannot cast to words type with odd length");
    return toWords(&src, sizeof(T)/2);
}

template <typename T>
QByteArray toBytes(const T &data) {
    return toBytes(&data, sizeof(T));
}

}

#endif // WORDOPERATIONS_H
