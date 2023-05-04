#include "serializableobject.h"

using namespace Serializable;

FieldConcept *Object::field(const QString &fieldName)
{
    return _priv_allFields().value(fieldName);
}

const FieldConcept *Object::field(const QString &fieldName) const
{
    return _priv_allFields().value(fieldName);
}

FieldConcept *Object::field(const IsFieldCheck &field)
{
    return _priv_allFields().value(findNameOf(field));
}

const FieldConcept *Object::field(const IsFieldCheck &field) const
{
    return _priv_allFields().value(findNameOf(field));
}

const QList<QString> &Object::fields() const
{
    return _priv_allFieldsNamesCached();
}

QString Object::findNameOf(const IsFieldCheck &rawField) const
{
    for (const auto &name: qAsConst(fields()))
    {
        if (field(name)->rawField(this) == &rawField) {
            return name;
        }
    }
    return {};
}

bool Object::update(const QVariantMap &source)
{
    bool wasUpdated = false;
    for (const auto &name: qAsConst(fields()))
    {
        if (source.contains(name)) {
            wasUpdated |= field(name)->updateWithVariant(this, source.value(name)); // stays true once set
        }
    }
    return wasUpdated;
}

QVariantMap Object::serialize() const
{
    QVariantMap result;
    for (const auto &fieldName: fields()) {
        auto found = field(fieldName);
        result.insert(fieldName, found->readVariant(this));
    }
    return result;
}

QVariantMap Object::schema() const
{
    QVariantMap result;
    for (auto iter = _priv_allFields().cbegin(); iter != _priv_allFields().cend(); ++iter) {
        result.insert(iter.key(), iter.value()->schema(this));
    }
    return result;

}

bool Object::is(const QMetaObject *mobj) const
{
    return metaObject()->inherits(mobj);
}

const void *Object::as(const QMetaObject *mobj) const
{
    if (is(mobj)) {
        return this;
    } else {
        return nullptr;
    }
}

void *Object::as(const QMetaObject *mobj)
{
    if (is(mobj)) {
        return this;
    } else {
        return nullptr;
    }
}
