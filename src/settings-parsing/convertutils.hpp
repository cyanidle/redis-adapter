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
QVariantMap convertToQMap(const QMap<QString, T> &src)
{
    static_assert(QMetaTypeId2<T>::Defined, "Convertion to non-registered type prohibited!");
    QVariantMap result;
    for (auto iter = src.constBegin(); iter != src.constEnd(); ++iter) {
        result.insert(iter.key(), QVariant::fromValue(iter.value()));
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

template<typename T>
QVariantList convertToQList(const QList<T> &src)
{
    static_assert(QMetaTypeId2<T>::Defined, "Convertion to non-registered type prohibited!");
    QVariantList result;
    for (auto iter = src.constBegin(); iter != src.constEnd(); ++iter) {
        result.append(QVariant::fromValue(*iter));
    }
    return result;
}
#endif // CONVERTUTILS_H
