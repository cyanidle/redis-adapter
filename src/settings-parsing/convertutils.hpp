#ifndef CONVERTUTILS_H
#define CONVERTUTILS_H

#include <QList>
#include <QVariantMap>
#include "private/global.h"

template<typename T>
QMap<QString, T> convertQMap(const QVariantMap &src)
{
    static_assert(QMetaTypeId2<T>::Defined, "Convertion to non-registered type prohibited!");
    QMap<QString, T> result;
    for (auto iter = src.constBegin(); iter != src.constEnd(); ++iter) {
        if (iter.value().canConvert<T>()) {
            result.insert(iter.key(), iter.value().value<T>());
        }
    }
    return result;
}
template<typename T>
QList<T> convertQList(const QVariantList &src)
{
    static_assert(QMetaTypeId2<T>::Defined, "Convertion to non-registered type prohibited!");
    QList<T> result;
    for (auto iter = src.constBegin(); iter != src.constEnd(); ++iter) {
        if (iter->canConvert<T>()) {
            result.append(iter->value<T>());
        }
    }
    return result;
}

#endif // CONVERTUTILS_H
