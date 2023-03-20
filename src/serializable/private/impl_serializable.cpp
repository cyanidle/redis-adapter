#include "impl_serializable.h"
#include "../serializable.h"

Serializable::NestedIntrospection::NestedIntrospection(Object *obj) :
    m_currentType(TypeObject), m_data(QVariant::fromValue(obj))
{}

Serializable::NestedIntrospection::NestedIntrospection(const QList<Object *> &obj) :
    m_currentType(TypeList), m_data(QVariant::fromValue(obj))
{}

Serializable::NestedIntrospection::NestedIntrospection(const QMap<QString, Object *> &obj) :
    m_currentType(TypeMap), m_data(QVariant::fromValue(obj))
{}

Serializable::Object *Serializable::NestedIntrospection::asObject() {
    return m_currentType == TypeObject ? m_data.value<Object*>() : nullptr;
}

QList<Serializable::Object *> Serializable::NestedIntrospection::asObjectsList() {
    return m_currentType == TypeList ? m_data.value<QList<Object*>>() : QList<Object*>();
}

QMap<QString, Serializable::Object *> Serializable::NestedIntrospection::asObjectsMap() {
    return m_currentType == TypeMap ? m_data.value<QMap<QString, Object*>>() : QMap<QString, Object*>();
}

const Serializable::Object *Serializable::NestedIntrospection::asObject() const {
    return m_currentType == TypeObject ? m_data.value<Object*>() : nullptr;
}

QList<const Serializable::Object *> Serializable::NestedIntrospection::asObjectsList() const {
    if (m_currentType != TypeList) {
        return {};
    }
    auto objsList = m_data.value<QList<Object*>>();
    return QList<const Object*>{objsList.begin(), objsList.end()};
}

QMap<QString, const Serializable::Object *> Serializable::NestedIntrospection::asObjectsMap() const {
    if (m_currentType != TypeMap) {
        return {};
    }
    QMap<QString, const Object*> result;
    auto objsMap = m_data.value<QMap<QString, Object*>>();
    for (auto iter = objsMap.begin(); iter != objsMap.end(); ++iter) {
        result.insert(iter.key(), *iter);
    }
    return result;
}
