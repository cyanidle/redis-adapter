#ifndef METATYPES_H
#define METATYPES_H

#include <QMetaType>

template <typename T>
constexpr bool metaIsRegistered() {
    return QMetaTypeId2<T>::Defined;
}
template <typename T, typename Ret = void>
using enabled_meta_t = typename std::enable_if<metaIsRegistered<T>(), Ret>::type;

#endif // METATYPES_H
