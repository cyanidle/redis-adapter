#include "wordoperations.h"
#include "templates/algorithms.hpp"

void flipBytesInWords(quint16 *words, int sizeWords) {
    for (int i = 0; i < sizeWords; i++) {
        quint8 bytes[2];
        std::memcpy(bytes, words + i, 2);
        std::swap(bytes[0], bytes[1]);
        std::memcpy(words + i, bytes, 2);
    }
}

QVector<quint16> ByteUtils::toWords(const void *src, int sizeWords, Order from, Order to) {
    QVector<quint16> result(sizeWords);
    std::memcpy(result.data(), src, sizeWords*2);
    if (from != to) {
        std::reverse(result.begin(), result.end());
    }
    return result;
}

QByteArray ByteUtils::toBytes(const void *src, int sizeBytes, Order from, Order to)
{
    QByteArray result(sizeBytes, Qt::Initialization::Uninitialized);
    std::memcpy(result.data(), src, sizeBytes);
    if (from != to) {
        std::reverse(result.begin(), result.end());
    }
    return result;
}

void ByteUtils::applyEndianess(quint16 *words, int sizeWords, const Settings::PackingMode endianessWas, const Settings::PackingMode targetEndianess)
{
    if (endianessWas.bytes != targetEndianess.bytes) {
        flipBytesInWords(words, sizeWords);
    }
    if (endianessWas.words != targetEndianess.words) {
        std::reverse(words, words + sizeWords);
    }
}
