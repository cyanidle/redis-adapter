#ifndef SERIALIZABLEOBJECT_H
#define SERIALIZABLEOBJECT_H

#include "common_fields.hpp"

namespace Serializable {

struct RADAPTER_API Object {
    Q_GADGET
public:
    FieldConcept *field(const QString &fieldName);
    const FieldConcept *field(const QString &fieldName) const;
    FieldConcept *field(const IsFieldCheck &field);
    const FieldConcept *field(const IsFieldCheck &field) const;
    const QList<QString> &fields() const;
    QString findNameOf(const IsFieldCheck &rawField) const;
    virtual bool update(const QVariantMap &source);
    virtual QVariantMap serialize() const;
    // full reflection info
    QVariantMap schema() const;
    // consice info, allows accessing schema() with keys. Values are type names
    QVariantMap structure() const;
    virtual const QMetaObject *metaObject() const = 0;
    virtual ~Object() = default;
    bool is(const QMetaObject *mobj) const;
    const void *as(const QMetaObject *mobj) const;
    void *as(const QMetaObject *mobj);
    template <class T> bool is() const;
    template <class T> T* as();
    template <class T> const T* as() const;
protected:
    virtual void postUpdate(){};
private:
    virtual const QMap<QString, FieldConcept*> &_priv_allFields() const = 0;
    virtual const QList<QString> &_priv_allFieldsNamesCached() const = 0;
};

template<class T>
bool Object::is() const
{
    return is(&T::staticMetaObject);
}

template<class T>
const T *Object::as() const
{
    return static_cast<const T *>(as(&T::staticMetaObject));
}

template<class T>
T *Object::as()
{
    return static_cast<T *>(as(&T::staticMetaObject));
}

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
