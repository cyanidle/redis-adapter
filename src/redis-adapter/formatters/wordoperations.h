#ifndef WORDOPERATIONS_H
#define WORDOPERATIONS_H

#include "redis-adapter/settings/modbussettings.h"
#include <cstring>
#include <QDataStream>

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

inline QVector<quint16> toWords(const void* src, int sizeWords) {
    quint16 result[sizeWords];
    std::memcpy(result, src, sizeWords*2);
    auto out = QVector<quint16>(sizeWords);
    for (int i = 0; i < sizeWords; ++i) {
        out[i] = result[i];
    }
    return out;
}

constexpr QDataStream::ByteOrder getEndianess() {
    return Q_BYTE_ORDER == Q_LITTLE_ENDIAN ? QDataStream::LittleEndian : QDataStream::BigEndian;
}

inline void applyToWords(quint16 *words, int sizeWords, QDataStream::ByteOrder wordOrder) {
    if (wordOrder != getEndianess()) {
        for (int i = 0; i < sizeWords/2; ++i) {
            std::swap(words[i], words[sizeWords - i - 1]);
        }
    }
}

inline void applyToBytes(quint16 *words, int sizeWords, QDataStream::ByteOrder byteOrder) {
    if (byteOrder != getEndianess()) {
        auto sizeBytes = sizeWords*2;
        quint8 bytes[sizeBytes];
        std::memcpy(bytes, words, sizeBytes);
        for (int i = 0; i < sizeBytes/2; ++i) {
            std::swap(bytes[i], bytes[sizeBytes - 1 - i]);
        }
        std::memcpy(words, bytes, sizeBytes);
    }
}

inline void applyEndianness(quint16 *words, const Settings::PackingMode endianess,
                            int sizeWords, bool receive) {
    if (!receive) {
        applyToBytes(words, sizeWords, endianess.byte_order);
    }
    applyToWords(words, sizeWords, endianess.word_order);
    if (receive) {
        applyToBytes(words, sizeWords, endianess.byte_order);
    }
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

#endif // WORDOPERATIONS_H
