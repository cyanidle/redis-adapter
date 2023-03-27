#include "wordoperations.h"
#include "templates/algorithms.hpp"

void ByteUtils::applyEndianness(quint16 *words, const Settings::PackingMode endianess, int sizeWords, bool receive) {
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

QVector<quint16> ByteUtils::toWords(const void *src, int sizeWords) {
    QVector<quint16> result(sizeWords);
    std::memcpy(result.data(), src, sizeWords*2);
    return result;
}

void ByteUtils::applyToBytes(quint16 *words, int sizeWords, QDataStream::ByteOrder byteOrder) {
    if (byteOrder != getEndianess()) {
        auto sizeBytes = sizeWords*2;
        QVector<quint8> bytes(sizeBytes);
        std::memcpy(bytes.data(), words, sizeBytes);
        std::reverse(bytes.begin(), bytes.end());
        std::memcpy(words, bytes.data(), sizeBytes);
    }
}

void ByteUtils::applyToWords(quint16 *words, int sizeWords, QDataStream::ByteOrder wordOrder) {
    if (wordOrder != getEndianess()) {
        std::reverse(words, words + sizeWords);
    }
}

QByteArray ByteUtils::toBytes(const void *src, int sizeBytes)
{
    QByteArray result(sizeBytes, Qt::Initialization::Uninitialized);
    std::memcpy(result.data(), src, sizeBytes);
    return result;
}
