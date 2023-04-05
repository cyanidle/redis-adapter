#ifndef UTILS_HPP
#define UTILS_HPP

#include <QVariant>

namespace Serializable {

template <typename T>
T fromQMap(const QVariantMap &source) {
    T res;
    res.update(source);
    return res;
}

template <typename T>
QList<T> fromQList(const QVariantList &source) {
    QList<T> result;
    for (const auto &subval : source) {
        auto asMap = subval.toMap();
        T temp;
        temp.update(asMap);
        result.append(temp);
    }
    return result;
}

}

#endif // UTILS_HPP
