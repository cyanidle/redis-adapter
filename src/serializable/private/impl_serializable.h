#ifndef IMPL_SERIALIZABLE_H
#define IMPL_SERIALIZABLE_H

#include <QVariant>
#include <QMetaProperty>
#include <QMetaObject>
#include <type_traits>
#include <stdexcept>
#include "templates/metaprogramming.hpp"

namespace Serializable {
struct FieldConcept;
struct Object;
struct NestedIntrospection {
    enum CurrentType {
        TypeInvalid = 0,
        TypeObject,
        TypeSequence,
        TypeMapping,
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
    QVariant m_data;
    CurrentType m_currentType;
};
namespace Private {
void *allocHelper(int size);
template <typename T>
void check_type() {
    constexpr auto isQGadget = Radapter::has_QGadget_Macro<T>::Value;
    static_assert(isQGadget, "Must use Q_GADGET macro!");
}
QStringList fieldNamesHelper(const Object *who);
QMap<QString, FieldConcept *> fieldsHelper(const Object *who);
}
}

#define FIELD(field_type, name, ...) \
    public: field_type name {__VA_ARGS__}; \
    private: \
    QVariant _priv_getFinalPtr_##name () { \
constexpr auto is_wrapped = ::std::is_base_of<::Serializable::IsFieldCheck, typename std::decay<decltype(name)>::type>(); \
static_assert(is_wrapped, "Dont use raw types in FIELD() macro"); \
        _has_Is_Serializable(); \
        return QVariant::fromValue(::Serializable::Private::upcastField(& THIS_TYPE :: name)); /* NOLINT*/ \
    } \
    Q_PROPERTY(QVariant __field__ ##name READ _priv_getFinalPtr_##name) \
    public:

#define RADAPTER_DECLARE_FIELD(name) \
    private: \
    QVariant _priv_getFinalPtr_##name () { \
        _has_Is_Serializable(); \
        return QVariant::fromValue(::Serializable::Private::upcastField(& THIS_TYPE :: name)); \
    } \
    Q_PROPERTY(QVariant __field__ ##name READ _priv_getFinalPtr_##name) \
    public:

#define _BASE_IS_SERIALIZABLE \
    private: \
    static void _has_Is_Serializable () noexcept {}; \
    virtual const QList<QString> &_priv_allFieldsNamesCached() const override { \
        static QStringList fieldsNames{::Serializable::Private::fieldNamesHelper(this)}; \
        return fieldsNames; \
    } \
    virtual const QMap<QString, ::Serializable::FieldConcept*> &_priv_allFields() const override { \
        static QMap<QString, ::Serializable::FieldConcept*> result{::Serializable::Private::fieldsHelper(this)}; \
        return result; \
    } \
    public: \
    virtual const QMetaObject *metaObject() const override { \
        ::Serializable::Private::check_type<THIS_TYPE>(); \
        return &this->staticMetaObject; \
    };

#define IS_SERIALIZABLE \
    _BASE_IS_SERIALIZABLE

#endif // SERIALIZER_H
