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
    virtual void postUpdate(){};
private:
    virtual const QMap<QString, FieldConcept*> &_priv_allFields() const = 0;
    virtual const QList<QString> &_priv_allFieldsNamesCached() const = 0;
};

#define POST_UPDATE virtual void postUpdate() override

template <typename T>
T parseObject(const QVariantMap &source) {
    T res;
    res.update(source);
    return res;
}

template <typename T>
QList<T> parseListOf(const QVariantList &source) {
    QList<T> result;
    for (const auto &subval : source) {
        auto asMap = subval.toMap();
        T temp;
        if (temp.update(asMap)) {
            result.append(temp);
        }
    }
    return result;
}

template <typename T>
QMap<QString, T> parseMapOf(const QVariantMap &source) {
    QMap<QString, T> result;
    for (auto iter = source.constBegin(); iter != source.constEnd(); ++iter) {
        auto asMap = iter->toMap();
        T temp;
        if (temp.update(asMap)) {
            result.insert(iter.key(), temp);
        }
    }
    return result;
}

} //namespace Serializable
Q_DECLARE_METATYPE(Serializable::Object*)
#endif // SERIALIZER_H
