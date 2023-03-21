#ifndef SERIALIZABLE_H
#define SERIALIZABLE_H

#include "common_fields.hpp"

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
    void fillFields() const;
    virtual void postUpdate(){};
    template <typename T>
    FieldConcept *upcastField(T *raw) {
        static_assert(std::is_base_of<FieldConcept, T>(), "Fields must inherit and implement FieldConcept!");
        return raw;
    }
private:
    virtual QVariant readProp(const QMetaProperty &prop) const = 0;
    virtual bool writeProp(const QMetaProperty &prop, const QVariant &val) = 0;

    mutable QMap<QString, FieldConcept*> m_fieldsMap;
    mutable QList<QString> m_fields;
};

#define POST_UPDATE virtual void postUpdate() override

using Gadget = GadgetMixin<Object>;
using Q_Object = QObjectMixin<Object>;

template <typename T>
T fromQMap(const QVariantMap &source) {
    T res;
    res.update(source);
    return res;
}

template <typename T>
QList<T> fromQList(const QVariantList &source) {
    QList<T> result;
    for (const auto &subval : source) {
        auto asMap = subval.toMap();
        T temp;
        temp.update(asMap);
        result.append(temp);
    }
    return result;
}
}
Q_DECLARE_METATYPE(Serializable::Object*)
#endif // SERIALIZER_H
