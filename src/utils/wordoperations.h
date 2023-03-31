#ifndef WORDOPERATIONS_H
#define WORDOPERATIONS_H

#include "settings/modbussettings.h"
#include <cstring>
#include <QDataStream>

namespace ByteUtils {

using Order = QDataStream::ByteOrder;

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

void applyEndianess(quint16 *words, int sizeWords, const Settings::PackingMode endianessWas, const Settings::PackingMode targetEndianess);

QVector<quint16> toWords(const void* src, int sizeWords, Order from = Order::BigEndian, Order to = Order::BigEndian);
QByteArray toBytes(const void* src, int sizeBytes, Order from = Order::BigEndian, Order to = getEndianess());

template<typename T>
inline QVector<quint16> toWords(const T& src, Order from = Order::BigEndian, Order to = Order::BigEndian) {
    static_assert(!(sizeof(T)%2), "Cannot cast to words type with odd length");
    return toWords(&src, sizeof(T)/2, from, to);
}

template <typename T>
QByteArray toBytes(const T &data, Order from = Order::BigEndian, Order to = getEndianess()) {
    return toBytes(&data, sizeof(T), from, to);
}

}

#endif // WORDOPERATIONS_H
