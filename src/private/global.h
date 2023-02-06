#ifndef RADAPTER_BROKER_GLOBAL_H
#define RADAPTER_BROKER_GLOBAL_H

#include <QObject>
#include <QMetaMethod>

#define RADAPTER_VERSION "0.3"

#ifndef RADAPTER_SHARED_SRC
#define RADAPTER_SHARED_SRC
#endif

namespace Radapter {
namespace Private {

template<typename T> struct has_QGadget_Macro {
    template <typename U>
    static char test(void (U::*)());
    static int test(void (T::*)());
    enum { Value =  sizeof(test(&T::qt_check_for_QGADGET_macro)) == sizeof(int) };
};

template<typename T> struct has_QObject_Macro {
    template <typename U>
    static char test(int (U::*)(QMetaObject::Call, int, void **));
    static int test(int (T::*)(QMetaObject::Call, int, void **));
    enum { Value =  sizeof(test(&T::qt_metacall)) == sizeof(int) };
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

#define GADGET_BODY \
    virtual const QMetaObject *metaObject() const override {return &this->staticMetaObject;};

template <typename T>
using stripped_this = typename std::decay<typename std::remove_pointer<T>::type>::type;
}

template<class... T>
void Unused(T...) noexcept {};
}


#endif
