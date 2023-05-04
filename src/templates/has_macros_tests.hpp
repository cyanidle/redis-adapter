#ifndef HAS_MACROS_TESTS_H
#define HAS_MACROS_TESTS_H
#include "private/global.h"
namespace Radapter
{
template<typename T> struct has_QObject_Macro {
    template<typename> static std::false_type impl(...);
    template<typename U> static auto impl(int) ->
        decltype(std::declval<U>().qt_metacall(QMetaObject::Call::IndexOfMethod, 1, nullptr), std::true_type());
    enum { Value = decltype(impl<T>(0))::value};
};

template<typename T> struct has_QGadget_Macro {
    template<typename> static std::false_type impl(...);
    template<typename U> static auto impl(int) ->
        decltype(std::declval<U>().qt_check_for_QGADGET_macro(), std::true_type());
    enum { Value = decltype(impl<T>(0))::value};
};

template<typename T> struct has_MetaObject_Func {
    template <typename U>
    static char test(const QMetaObject * (U::*)() const);
    static int test(const QMetaObject * (T::*)() const);
    enum { Value =  sizeof(test(&T::metaObject)) == sizeof(int) };
};

template<typename T>
struct Gadget_With_MetaObj {
    enum { Value = has_MetaObject_Func<T>::Value &&
                   has_QGadget_Macro<T>::Value};
};

}
#endif // HAS_MACROS_TESTS_H
