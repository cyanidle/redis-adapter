#include "impl_serializable.h"
#include "../serializableobject.h"

Serializable::NestedIntrospection::NestedIntrospection(Object *obj) :
    m_data(QVariant::fromValue(obj)), m_currentType(TypeObject)
{}

Serializable::NestedIntrospection::NestedIntrospection(const QList<Object *> &obj) :
    m_data(QVariant::fromValue(obj)), m_currentType(TypeSequence)
{}

Serializable::NestedIntrospection::NestedIntrospection(const QMap<QString, Object *> &obj) :
    m_data(QVariant::fromValue(obj)), m_currentType(TypeMapping)
{}

Serializable::Object *Serializable::NestedIntrospection::asObject() {

    if (m_currentType != TypeObject) {
        throw std::runtime_error("Field introspection type missmatch!");
    }
    return m_data.value<Object*>();
}

QList<Serializable::Object *> Serializable::NestedIntrospection::asObjectsList() {

    if (m_currentType != TypeSequence) {
        throw std::runtime_error("Field introspection type missmatch!");
    }
    return m_data.value<QList<Object*>>();
}

QMap<QString, Serializable::Object *> Serializable::NestedIntrospection::asObjectsMap() {

    if (m_currentType != TypeMapping) {
        throw std::runtime_error("Field introspection type missmatch!");
    }
    return m_data.value<QMap<QString, Object*>>();
}

const Serializable::Object *Serializable::NestedIntrospection::asObject() const {

    if (m_currentType != TypeObject) {
        throw std::runtime_error("Field introspection type missmatch!");
    }
    return m_data.value<Object*>();
}

QList<const Serializable::Object *> Serializable::NestedIntrospection::asObjectsList() const {
    if (m_currentType != TypeSequence) {
        throw std::runtime_error("Field introspection type missmatch!");
    }
    auto objsList = m_data.value<QList<Object*>>();
    return QList<const Object*>{objsList.begin(), objsList.end()};
}

QMap<QString, const Serializable::Object *> Serializable::NestedIntrospection::asObjectsMap() const {
    if (m_currentType != TypeMapping) {
        throw std::runtime_error("Field introspection type missmatch!");
    }
    QMap<QString, const Object*> result;
    auto objsMap = m_data.value<QMap<QString, Object*>>();
    for (auto iter = objsMap.begin(); iter != objsMap.end(); ++iter) {
        result.insert(iter.key(), *iter);
    }
    return result;
}

namespace Serializable {
QMap<QString, FieldConcept*> Private::fieldsHelper(const Object *who)
{
    constexpr QLatin1String start("__field__", 9);
    auto mobj = who->metaObject();
    auto props = mobj->propertyCount();
    QMap<QString, FieldConcept*> result;
    for (int i = 0; i < props ; i ++) {
        auto field = mobj->property(i);
        auto name = QLatin1String(field.name());
        if (name.startsWith(start)) {
            auto value = field.readOnGadget(who).value<FieldConcept*>();
            result.insert(name.mid(start.size()), value);
        }
    }
    return result;
}

QStringList Private::fieldNamesHelper(const Object *who)
{
    constexpr QLatin1String start("__field__", 9);
    auto mobj = who->metaObject();
    auto props = mobj->propertyCount();
    QStringList result;
    for (int i = 0; i < props ; i ++) {
        auto field = mobj->property(i);
        auto name = QLatin1String(field.name());
        if (name.startsWith(start)) {
            result.append(name.mid(start.size()));
        }
    }
    return result;
}

void *Private::allocHelper(int size)
{
    return malloc(size);
}

}
