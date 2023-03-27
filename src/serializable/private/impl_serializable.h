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
struct Object;
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
    QVariant m_data;
    CurrentType m_currentType;
};
namespace Private {
template <typename T>
void check_type() {
    constexpr auto isQGadget = Radapter::has_QGadget_Macro<T>::Value;
    static_assert(isQGadget, "Must use Q_GADGET macro!");
}
QMap<QString, FieldConcept*> fieldsHelper(const Object *who);
}

#define FIELD(field_type, name, ...) \
    public: field_type name __VA_ARGS__; \
    private: \
    decltype(name)* _priv_getPtr_##name () {return &name;} \
    QVariant _priv_getFinalPtr_##name () { \
        _has_Is_Serializable(); \
        return QVariant::fromValue(::Serializable::Private::upcastField(& THIS_TYPE :: _priv_getPtr_##name)); \
    } \
    Q_PROPERTY(QVariant __field__ ##name READ _priv_getFinalPtr_##name) \
    public:

#define IS_SERIALIZABLE \
    private: \
    static void _has_Is_Serializable () noexcept {}; \
    virtual const QList<QString> &_priv_allFieldsNamesCached() const override { \
        static QStringList fieldsNames{_priv_allFields().keys()}; \
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

}

#endif // SERIALIZER_H
