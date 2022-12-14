#ifndef WORDOPERATIONS_H
#define WORDOPERATIONS_H

#include "redis-adapter/settings/modbussettings.h"
#include <cstring>
#include <QDataStream>

template<typename T>
typename std::enable_if<!(std::is_pointer<T>())>::type
flipWords(T& target) {
    static_assert(!(sizeof(T)%2), "Odd words count types unsupported!");
    static_assert(sizeof(T)>=2, "Size of target must be >= 2 bytes (1 word)!");
    constexpr const auto sizeWords = sizeof(T)/2;
    quint16 words[sizeof(T)/2];
    std::memcpy(words, &target, sizeof(T));
    for (size_t i = 0; i < sizeWords / 2; ++i) {
        std::swap(words[i], words[sizeWords - 1 - i]);
    }
    std::memcpy(&target, words, sizeof(T));
}

inline void flipBytes(quint16 *words, int sizeWords) {
    quint8 bytes[sizeWords*2];
    std::memcpy(bytes, words, sizeWords*2);
    for (int i = 0; i < sizeWords; ++i) {
        std::swap(bytes[i], bytes[sizeWords - 1 - i]);
    }
    std::memcpy(words, bytes, sizeWords*2);
}

inline void applyEndianness(quint16 *words, const Settings::PackingMode endianess, int sizeWords) {
    if (endianess.byte_order ==
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        QDataStream::BigEndian
#else
        QDataStream::LittleEndian
#endif
        ) {
        flipBytes(words, sizeWords);
    }
    if (endianess.word_order ==
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        QDataStream::LittleEndian
#else
        QDataStream::BigEndian
#endif
        ) {
        flipWords(*words);
    }
}

template <typename To, typename From>
typename std::enable_if<std::is_trivially_copyable<To>::value &&
                        std::is_trivially_copyable<From>::value &&
                        !std::is_pointer<From>::value, To>::type
bit_cast(const From* src) {
    To result;
    std::memcpy(&result, src, sizeof(To));
    return result;
}

template <typename To, typename From>
typename std::enable_if<sizeof(To) == sizeof(From) &&
                        std::is_trivially_copyable<To>::value &&
                        std::is_trivially_copyable<From>::value &&
                        !std::is_pointer<From>::value, To>::type
bit_cast(const From& src) {
    To result;
    std::memcpy(&result, &src, sizeof(To));
    return result;
}

#endif // WORDOPERATIONS_H
