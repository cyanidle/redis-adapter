#ifndef COMMON_FIELDS_H
#define COMMON_FIELDS_H

#include "private/impl_serializable.hpp"
namespace Serializable {

struct FieldConcept {
    virtual QVariant schema() const = 0;
    virtual QVariant read() const = 0;
    virtual bool set(const QVariant &source) = 0;
    virtual int type() const = 0;
    virtual ~FieldConcept() = default;
};

template<typename T>
struct Field : public FieldConcept {
    Field() : value() {}
    Field(const Field& other) : value(other.value) {}
    Field &operator=(const Field &other){value = other.value;}
    operator T&() {return value;}
    operator const T&() const {return value;}
    QVariant schema() const override {
        static QMetaType metaType = QMetaType::fromType<T>();
        static QVariantMap actual_schema {
            {"type_id", metaType.id()},
            {"type_name", metaType.name()},
            {"value", QVariant::fromValue(value)},
        };
        return actual_schema;
    }
    QVariant read() const override {
        return QVariant::fromValue(value);
    }
    bool set(const QVariant &source) override {
        if (source.canConvert<T>()) {
            value = source.value<T>();
            return true;
        }
        return false;
    }
    T value;
};

}

#endif // SERIALIZER_H
