#ifndef COMMON_FIELDS_H
#define COMMON_FIELDS_H

#include "private/impl_serializable.hpp"
#include "radapterlogging.h"

namespace Serializable {
    struct FieldConcept;
    struct Object;
    Q_NAMESPACE_EXPORT(RADAPTER_SHARED_SRC)
    enum FieldType {
        FieldInvalid = 0,
        FieldPlain,
        FieldContainer,
        FieldMap,
        FieldNested,
        FieldContainerOfNested,
        FieldMapOfNested
    };
    Q_ENUM_NS(FieldType)
}
Q_DECLARE_METATYPE(Serializable::FieldConcept*)
namespace Serializable {
struct FieldConcept {
    virtual QVariant schema() const = 0;
    virtual QVariant readVariant() const = 0;
    virtual bool updateVariant(const QVariant &source) = 0;
    virtual FieldType type() const = 0;
    virtual ~FieldConcept() = default;
    template <typename Target>
    Target *as() {return Target::fieldType == type() ? this : nullptr;}
    template <typename Target>
    const Target *as() const {return Target::Type == type() ? this : nullptr;}
};

template<typename T, typename = void>
struct Field;

namespace Private {
template<typename T, bool isBig = true>
struct FieldCommon : public FieldConcept {
    using valueType = T;
    using valueRef = typename std::conditional<isBig, T, T&>;
    FieldCommon() : value() {}
    FieldCommon(const FieldCommon& other) : value(other.value) {}
    FieldCommon &operator=(const FieldCommon &other){value = other.value;}
    operator T&() {return value;}
    operator const T&() const {return value;}
    void update(const valueRef newVal) {
        this->value = newVal;
    }
    T value;
};
struct BindableSignals : public QObject {
    Q_OBJECT
signals:
    void wasUpdated();
    void updateFailed();
};
}

template<typename T>
struct Field<T, typename std::enable_if<!std::is_base_of<Object, T>::value>::type> :
        public Private::FieldCommon<T, (sizeof(T) > sizeof(void*))>
{
    using typename Private::FieldCommon<T>::valueType;
    using typename Private::FieldCommon<T>::valueRef;
    constexpr static FieldType fieldType{FieldPlain};
    FieldType type() const override {return fieldType;}
    QVariant schema() const override {
        QMetaType metaType = QMetaType::fromType<T>();
        QVariantMap actual_schema {
            {"type_id", metaType.id()},
            {"type_name", metaType.name()},
            {"field_type", fieldType},
            {"field_type_name", QMetaEnum::fromType<FieldType>().valueToKey(fieldType)},
            {"value", readVariant()},
        };
        return actual_schema;
    }
    QVariant readVariant() const override {
        return QVariant::fromValue(this->value);
    }
    bool updateVariant(const QVariant &source) override {
        if (source.canConvert<T>()) {
            this->value = source.value<T>();
            if (this->value != source)
            settingsParsingWarn().noquote() << "Types mismatch, while deserializing -->\n Wanted: " <<
                                               QMetaType::fromType<T>().name() <<
                                               "\n Actual: " << source;
            return true;
        }
        return false;
    }
};

template<typename T>
struct Field<T, typename std::enable_if<std::is_base_of<Object, T>::value>::type> : public Private::FieldCommon<T> {
    using typename Private::FieldCommon<T>::valueType;
    using typename Private::FieldCommon<T>::valueRef;
    constexpr static FieldType fieldType{FieldPlain};
    FieldType type() const override {return fieldType;}
    QVariant schema() const override {
        QMetaType metaType = QMetaType::fromType<T>();
        QVariantMap merged;
        for (const auto &field : this->value.fields()) {
            merged.insert(field, this->value.field(field).schema());
        }
        QVariantMap actual_schema {
            {"type_name", metaType.name()},
            {"field_type", fieldType},
            {"field_type_name", QMetaEnum::fromType<FieldType>().valueToKey(fieldType)},
            {"fields", merged},
            {"value", readVariant()}
        };
        return actual_schema;
    }
    QVariant readVariant() const override {
        return this->value.serialize();
    }
    bool updateVariant(const QVariant &source) override {
        return this->value.deserialize(source);
    }
};

template<typename T, typename = void>
struct Container;

template<typename T>
struct Container<T, typename std::enable_if<!std::is_base_of<Object, T>::value>::type> : public Private::FieldCommon<QList<T>> {
    using typename Private::FieldCommon<QList<T>>::valueType;
    using typename Private::FieldCommon<QList<T>>::valueRef;
    constexpr static FieldType fieldType{FieldContainer};
    FieldType type() const override {return fieldType;}
    QVariant schema() const override {
        QMetaType metaType = QMetaType::fromType<T>();
        QMetaType nestedMeta = QMetaType::fromType<decltype(this->value.first())>();
        QVariantMap actual_schema {
            {"type_name", metaType.name()},
            {"nested_type_name", nestedMeta.name()},
            {"nested_type_id", nestedMeta.id()},
            {"field_type", fieldType},
            {"field_type_name", QMetaEnum::fromType<FieldType>().valueToKey(fieldType)},
            {"value", readVariant()},
        };
        return actual_schema;
    }
    QVariant readVariant() const override {
        QVariantList values;
        for (const auto &value : this->value) {
            values.append(QVariant::fromValue(value));
        }
        return values;
    }
    bool updateVariant(const QVariant &source) override {
        if (source.canConvert<T>()) {
            this->value = source.value<T>();
            settingsParsingWarn().noquote() << "Types mismatch, while deserializing -->\n Wanted: " <<
                                               QMetaType::fromType<T>().name() <<
                                               "\n Actual: " << source;
            return true;
        }
        return false;
    }
};

template<typename Target>
struct Bindable : public Private::BindableSignals, public Target {
    template <typename...Args>
    QMetaObject::Connection onUpdate(Args&&...args) {
        return connect(this, &Private::BindableSignals::wasUpdated, std::forward<Args>(args)...);
    }
    bool updateVariant(const QVariant &source) override {
        auto status = Target::updateVariant(source);
        if (status) {
            emit wasUpdated();
        } else {
            emit updateFailed();
        }
        return status;
    }
};

}


#endif // SERIALIZER_H
