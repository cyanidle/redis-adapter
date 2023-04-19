#include "impl_serializable.h"
#include "../serializable.h"

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
    return m_currentType == TypeObject ? m_data.value<Object*>() : nullptr;
}

QList<Serializable::Object *> Serializable::NestedIntrospection::asObjectsList() {
    return m_currentType == TypeSequence ? m_data.value<QList<Object*>>() : QList<Object*>();
}

QMap<QString, Serializable::Object *> Serializable::NestedIntrospection::asObjectsMap() {
    return m_currentType == TypeMapping ? m_data.value<QMap<QString, Object*>>() : QMap<QString, Object*>();
}

const Serializable::Object *Serializable::NestedIntrospection::asObject() const {
    return m_currentType == TypeObject ? m_data.value<Object*>() : nullptr;
}

QList<const Serializable::Object *> Serializable::NestedIntrospection::asObjectsList() const {
    if (m_currentType != TypeSequence) {
        return {};
    }
    auto objsList = m_data.value<QList<Object*>>();
    return QList<const Object*>{objsList.begin(), objsList.end()};
}

QMap<QString, const Serializable::Object *> Serializable::NestedIntrospection::asObjectsMap() const {
    if (m_currentType != TypeMapping) {
        return {};
    }
    QMap<QString, const Object*> result;
    auto objsMap = m_data.value<QMap<QString, Object*>>();
    for (auto iter = objsMap.begin(); iter != objsMap.end(); ++iter) {
        result.insert(iter.key(), *iter);
    }
    return result;
}

namespace Serializable {
QMap<QString, QSharedPointer<FieldConcept>> Private::fieldsHelper(const Object *who)
{
    constexpr QLatin1String start("__field__", 9);
    auto mobj = who->metaObject();
    auto props = mobj->propertyCount();
    QMap<QString, QSharedPointer<FieldConcept>> result;
    for (int i = 0; i < props ; i ++) {
        auto field = mobj->property(i);
        auto name = QLatin1String(field.name());
        if (name.startsWith(start)) {
            auto value = field.readOnGadget(who).value<QSharedPointer<FieldConcept>>();
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
