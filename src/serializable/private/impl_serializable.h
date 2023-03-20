#ifndef IMPL_SERIALIZABLE_H
#define IMPL_SERIALIZABLE_H

#include <QVariant>
#include <QMetaProperty>
#include <QMetaObject>
#include <type_traits>
#include <stdexcept>
#include "templates/metaprogramming.hpp"
#include <boost/preprocessor/if.hpp>
#include <boost/preprocessor/comma_if.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/list/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/variadic/to_list.hpp>
#include <boost/preprocessor/stringize.hpp>

namespace Serializable {
struct FieldConcept;
template <class Target>
struct GadgetMixin : public Target
{
public:
    template <typename...Forward>
    GadgetMixin(Forward...args) :
        Target(args...)
    {}
protected:
    QVariant readProp(const QMetaProperty &prop) const override final {
        return prop.readOnGadget(this);
    }
    bool writeProp(const QMetaProperty &prop, const QVariant &val) override final {
        return prop.writeOnGadget(this, val);
    }
};

template <class Target>
struct QObjectMixin : public QObject, public Target
{
public:
    template <typename...Forward>
    QObjectMixin(QObject *parent = nullptr, Forward...args) :
        QObject(parent),
        Target(args...)
    {}
protected:
    QVariant readProp(const QMetaProperty &prop) const override final {
        return prop.read(this);
    }
    bool writeProp(const QMetaProperty &prop, const QVariant &val) override final {
        return prop.write(this, val);
    }
};
struct Object;
}
namespace Serializable {
struct NestedIntrospection {
    enum CurrentType {
        TypeInvalid = 0,
        TypeObject,
        TypeList,
        TypeMap,
    };
    NestedIntrospection() : m_currentType(TypeInvalid) {}
    NestedIntrospection(Object* obj);
    NestedIntrospection(const QList<Object*> &obj);
    NestedIntrospection(const QMap<QString, Object*> &obj);
    CurrentType currentType() const {return m_currentType;}
    Object* asObject();
    QList<Object*> asObjectsList();
    QMap<QString, Object*> asObjectsMap();
    const Object* asObject() const;
    QList<const Object*> asObjectsList() const;
    QMap<QString, const Object*> asObjectsMap() const;
private:
    CurrentType m_currentType;
    QVariant m_data;
};
namespace Private {
template <typename T>
void check_type() {
    constexpr auto isQObject = Radapter::has_QGadget_Macro<T>::Value;
    constexpr auto isQGadget = Radapter::has_QObject_Macro<T>::Value && std::is_base_of<QObject, T>::value;
    static_assert(isQGadget || isQObject, "Please add Q_GADGET or Q_OBJECT macro!");
}
inline void check_field(const FieldConcept *field) {Q_UNUSED(field)}
}

#define _PROP_NAME(...) BOOST_PP_LIST_CAT(BOOST_PP_VARIADIC_TO_LIST(__fields__, __VA_ARGS__))
#define _IMPL_FIELD(field) {#field, upcastField(&field)}
#define _CHECK_FIELD(r, macro, i, elem) Serializable::Private::check_field(upcastField(&elem));
#define _VARIADIC_MAP(r, macro, i, elem) BOOST_PP_COMMA_IF(i) macro(elem)
#define _IMPL_FIELDS(...) BOOST_PP_SEQ_FOR_EACH_I(_VARIADIC_MAP, _IMPL_FIELD, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
#define _IMPL_CHECK_FIELDS(...) BOOST_PP_SEQ_FOR_EACH_I(_CHECK_FIELD, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
#define _FIELDS_BASE(...) \
    private: \
    Q_PROPERTY(QVariant _PROP_NAME(__VA_ARGS__) READ _priv_fields) \
    QVariant _priv_fields() { \
        _IMPL_CHECK_FIELDS(__VA_ARGS__); \
        using this_type = Radapter::stripped_this<decltype(this)>; \
        Serializable::Private::check_type<this_type>(); \
        return QVariant::fromValue(QMap<QString, Serializable::FieldConcept*>{_IMPL_FIELDS(__VA_ARGS__)}); \
    } public:

#define FIELDS(...) _FIELDS_BASE(__VA_ARGS__) \
    virtual const QMetaObject *metaObject() const override {return &this->staticMetaObject;}

#define FIELDS_QOBJECT(...) _FIELDS_BASE(__VA_ARGS__)


}

#endif // SERIALIZER_H
