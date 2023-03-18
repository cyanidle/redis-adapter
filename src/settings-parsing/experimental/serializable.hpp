#ifndef SERIALIZABLE_H
#define SERIALIZABLE_H

#include "private/impl_serializable.hpp"
#include "common_fields.hpp"

namespace Serializable {

struct ObjectBase {
    Q_GADGET
public:
    enum Types {
        Invalid = 0,
        FieldPlain,
        Container,
        Map,
        FieldNested,
        ContainerOfNested,
        MapOfNested
    };
    Q_ENUM(Types)
private:
    virtual const FieldsCache &fieldsCache() const = 0;
    virtual QVariant readProp(const QMetaProperty &prop) const = 0;
    virtual bool writeProp(const QMetaProperty &prop, const QVariant &val) = 0;
};

using Gadget = GadgetMixin<ObjectBase>;
using Object = QObjectMixin<ObjectBase>;

struct Test : Gadget {
    Q_GADGET
    FIELDS(a, b)
    Field<int> a;
    Field<int> b;
};

void lol() {
    auto a = Test();
    auto b = a.a;
}
}

#endif // SERIALIZER_H
