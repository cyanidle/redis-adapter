#ifndef SERIALIZABLE_H
#define SERIALIZABLE_H

#include "common_fields.hpp"
#include "utils.hpp"

namespace Serializable {

struct RADAPTER_API Object {
    FieldConcept *field(const QString &fieldName);
    const FieldConcept *field(const QString &fieldName) const;
    const QList<QString> &fields() const;
    virtual bool update(const QVariantMap &source);
    virtual QVariantMap serialize() const;
    QVariantMap schema() const;
    virtual const QMetaObject *metaObject() const = 0;
    virtual ~Object() = default;
protected:
    virtual void postUpdate(){};
private:
    virtual const QMap<QString, FieldConcept*> &_priv_allFields() const = 0;
    virtual const QList<QString> &_priv_allFieldsNamesCached() const = 0;
};

#define POST_UPDATE virtual void postUpdate() override

}
Q_DECLARE_METATYPE(Serializable::Object*)
#endif // SERIALIZER_H
