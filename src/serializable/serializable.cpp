#include "serializable.h"

using namespace Serializable;

FieldConcept *Object::field(const QString &fieldName)
{
    fillFields();
    return m_fieldsMap.value(fieldName);
}

const FieldConcept *Object::field(const QString &fieldName) const
{
    fillFields();
    return m_fieldsMap.value(fieldName);
}

const QList<QString> &Object::fields() const
{
    fillFields();
    return m_fields;
}

bool Object::update(const QVariantMap &source)
{
    fillFields();
    bool wasUpdated = false;
    for (auto iter = source.cbegin(); iter != source.cend(); ++iter) {
        auto found = m_fieldsMap.value(iter.key());
        if (found) {
            wasUpdated = found->updateWithVariant(iter.value()) || wasUpdated; // stays true once set
        } else {
            wasUpdated = wasUpdated || false;
        }
    }
    return wasUpdated; // --> was updated? true/false
}

QVariantMap Object::serialize() const
{
    fillFields();
    QVariantMap result;
    auto copy = m_fieldsMap;
    for (auto iter = copy.cbegin(); iter != copy.cend(); ++iter) {
        result.insert(iter.key(), iter.value()->readVariant());
    }
    return result;
}

QVariantMap Object::schema() const
{
    fillFields();
    QVariantMap result;
    auto copy = m_fieldsMap;
    for (auto iter = copy.cbegin(); iter != copy.cend(); ++iter) {
        result.insert(iter.key(), iter.value()->schema());
    }
    return result;

}

void Object::fillFields() const
{
    if (!m_fields.isEmpty()) return;
    auto mobj = metaObject();
    auto props = mobj->propertyCount();
    for (auto i = 0; i < props; i++) {
        auto prop = mobj->property(i);
        if (!QString(prop.name()).startsWith(QStringLiteral("__fields__"))) continue;
        auto fields = readProp(prop).value<QMap<QString, FieldConcept*>>();
        for (auto iter = fields.cbegin(); iter != fields.cend(); ++iter) {
            m_fieldsMap.insert(iter.key(), iter.value());
            m_fields.append(iter.key());
        }
    }
}
