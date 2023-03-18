#ifndef IMPL_SERIALIZABLE_H
#define IMPL_SERIALIZABLE_H

#include <QVariant>
#include <QMetaProperty>
#include <QMetaObject>
#include <type_traits>
#include <stdexcept>
#include "templates/metaprogramming.hpp"
#include <boost/preprocessor/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

namespace Serializable {

struct FieldConcept;
struct FieldsCache {
    QMap<QString, FieldConcept*> cache{};
    bool wasInit{false};
};

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

#define _IMPL_FIELD(field) \
    {#field, &field}
#define VARIADIC_MAP(r, macro, i, elem) BOOST_PP_COMMA_IF(i) macro(elem)
#define _IMPL_FIELDS(...) BOOST_PP_SEQ_FOR_EACH_I(VARIADIC_MAP, _IMPL_FIELD, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
#define FIELDS(...) \
    private: \
    Q_PROPERTY(QMap<QString, const FieldConcept*> __fields__##__VA_ARGS__ READ _this_fields) \
    virtual const FieldsCache &fieldsCache() const override { \
       using this_type = Radapter::stripped_this<decltype(this)>; \
       constexpr auto isQObject = Radapter::has_QGadget_Macro<this_type>::Value; \
       constexpr auto isQGadget = Radapter::has_QObject_Macro<this_type>::Value && std::is_base_of<QObject, this_type>::value; \
       static_assert(isQGadget || isQObject, ""); \
       return _priv_fields_cache;\
    } \
    FieldsCache _priv_fields_cache; \
    QMap<QString, const FieldConcept*> _this_fields() const { \
        return {_IMPL_FIELDS(__VA_ARGS__)}; \
    } public:

}

#endif // SERIALIZER_H
