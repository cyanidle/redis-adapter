#include "wordoperations.h"
#include "templates/algorithms.hpp"

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
