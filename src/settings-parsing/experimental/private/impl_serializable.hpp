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
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/stringize.hpp>
namespace Serializable {

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
#define _IMPL_FIELD(field) {#field, &field}
#define VARIADIC_MAP(r, macro, i, elem) BOOST_PP_COMMA_IF(i) macro(elem)
#define _IMPL_FIELDS(...) BOOST_PP_SEQ_FOR_EACH_I(VARIADIC_MAP, _IMPL_FIELD, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
#define FIELDS(...) \
    private: \
    virtual const QMetaObject *metaObject() const override {return &this->staticMetaObject;} \
    Q_PROPERTY(QVariant __fields__ READ _priv_fields) \
    QVariant _priv_fields() { \
        using this_type = Radapter::stripped_this<decltype(this)>; \
        constexpr auto isQObject = Radapter::has_QGadget_Macro<this_type>::Value; \
        constexpr auto isQGadget = Radapter::has_QObject_Macro<this_type>::Value && std::is_base_of<QObject, this_type>::value; \
        static_assert(isQGadget || isQObject, "Please add Q_GADGET or Q_OBJECT macro!"); \
        return QVariant::fromValue(QMap<QString, Serializable::FieldConcept*>{_IMPL_FIELDS(__VA_ARGS__)}); \
    } public:

}

#endif // SERIALIZER_H
