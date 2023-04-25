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

const QList<QString> &Object::fields() const
{
    return _priv_allFieldsNamesCached();
}

bool Object::update(const QVariantMap &source)
{
    bool wasUpdated = false;
    for (auto iter = source.cbegin(); iter != source.cend(); ++iter) {
        if (auto found = _priv_allFields().value(iter.key())) {
            wasUpdated |= found->updateWithVariant(this, iter.value()); // stays true once set
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
