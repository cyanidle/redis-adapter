#ifndef SERIALIZABLE_H
#define SERIALIZABLE_H

#include "private/impl_serializable.hpp"
#include "common_fields.hpp"

namespace Serializable {

struct Object {
    Q_GADGET
public:
    FieldConcept *field(const QString &fieldName);
    const FieldConcept *field(const QString &fieldName) const;
    const QList<QString> &fields() const;
    bool deserialize(const QVariantMap &source);
    QVariantMap serialize() const;
    virtual const QMetaObject *metaObject() const = 0;
private:
    void fillFields() const;
    virtual QVariant readProp(const QMetaProperty &prop) const = 0;
    virtual bool writeProp(const QMetaProperty &prop, const QVariant &val) = 0;

    mutable QMap<QString, FieldConcept*> m_fieldsMap;
    mutable QList<QString> m_fields;
    bool m_wasCacheFilled{false};
};

using Gadget = GadgetMixin<Object>;
using Q_Object = QObjectMixin<Object>;

}

#endif // SERIALIZER_H
