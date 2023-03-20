#ifndef COMMON_FIELDS_H
#define COMMON_FIELDS_H

#include "private/impl_serializable.h"
#include "radapterlogging.h"

namespace Serializable {
    struct FieldConcept;
    struct Object;
    Q_NAMESPACE_EXPORT(RADAPTER_API)
    enum FieldType {
        FieldInvalid = 0,
        FieldPlain,
        FieldSequence,
        FieldMapping,
        FieldNested,
        FieldSequenceOfNested,
        FieldMappingOfNested
    };
    Q_ENUM_NS(FieldType)
}
Q_DECLARE_METATYPE(Serializable::FieldConcept*)
namespace Serializable {

struct FieldConcept {
    virtual QVariant schema() const = 0;
    virtual QVariant readVariant() const = 0;
    virtual bool updateWithVariant(const QVariant &source) = 0;
    virtual FieldType type() const = 0;
    QString typeName() const {return QMetaEnum::fromType<FieldType>().valueToKey(type());}
    virtual int valueMetaTypeId() const = 0;
    virtual void *rawValue() = 0;
    virtual const void *rawValue() const = 0;
    virtual const QStringList &attributes() const {static QStringList attrs; return attrs;}
    virtual const NestedIntrospection introspectNested() const {return NestedIntrospection();}
    virtual NestedIntrospection introspectNested() {return NestedIntrospection();}
    bool isNested() const {return type() == FieldNested || type() == FieldSequenceOfNested || type() == FieldMappingOfNested;};
    virtual ~FieldConcept() = default;
};

namespace Private {
template<typename T, bool isBig = true>
struct FieldCommon : public FieldConcept {
    using valueType = T;
    using valueRef = typename std::conditional<isBig, T, T&>::type;
    FieldCommon() : value() {}
    template<typename U>
    FieldCommon(U&&other) : value(std::forward<U>(other)) {}
    template<typename U>
    FieldCommon &operator=(U&&other){value = std::forward<U>(other); return *this;}
    T* operator->() {return &value;}
    const T* operator->() const {return &value;}
    T& operator*() {return value;}
    const T& operator*() const {return value;}
    operator T&() {return value;}
    operator const T&() const {return value;}
    operator QVariant() const {return QVariant(value);}
    void *rawValue() override {return &value;}
    const void *rawValue() const override {return &value;}
    void update(const valueRef newVal) {
        this->value = newVal;
    }
    T value;
};
}

template<typename T>
struct PlainField : public Private::FieldCommon<T, (sizeof(T) > sizeof(void*))>
{
    using typename Private::FieldCommon<T, (sizeof(T) > sizeof(void*))>::valueType;
    using typename Private::FieldCommon<T, (sizeof(T) > sizeof(void*))>::valueRef;
    using Private::FieldCommon<T, (sizeof(T) > sizeof(void*))>::FieldCommon;
    using Private::FieldCommon<T, (sizeof(T) > sizeof(void*))>::operator=;
    using Private::FieldCommon<T, (sizeof(T) > sizeof(void*))>::value;
    static int staticValueMetaTypeId() {return QMetaType::fromType<T>().id();}
    int valueMetaTypeId() const override {return staticValueMetaTypeId();}
    enum {fieldType = FieldPlain};
    FieldType type() const override {return static_cast<FieldType>(fieldType);}
    QVariant schema() const override {
        QVariantMap actual_schema {
            {"type_id", QMetaType::fromType<T>().id()},
            {"type_name", QMetaType::fromType<T>().name()},
            {"field_type", fieldType},
            {"field_type_name", QMetaEnum::fromType<FieldType>().valueToKey(fieldType)},
            {"value", readVariant()},
        };
        return actual_schema;
    }
    QVariant readVariant() const override {
        return QVariant::fromValue(this->value);
    }
    bool updateWithVariant(const QVariant &source) override {
        if (source.canConvert<T>()) {
            auto temp = source.value<T>();
            if (QVariant::fromValue(temp) != source) {
                settingsParsingWarn().noquote() << "Types mismatch, while deserializing -->\n Wanted: " <<
                                                   QMetaType::fromType<T>().name() <<
                                                   "\n Actual: " << source;
                return false;
            } else {
                this->value = temp;
                return true;
            }
        }
        return false;
    }
};

template<typename T>
struct NestedField : public Private::FieldCommon<T> {
    using typename Private::FieldCommon<T>::valueType;
    using typename Private::FieldCommon<T>::valueRef;
    using Private::FieldCommon<T>::value;
    using Private::FieldCommon<T>::FieldCommon;
    using Private::FieldCommon<T>::operator=;
    static int staticValueMetaTypeId() {return -1;}
    int valueMetaTypeId() const override {return staticValueMetaTypeId();}
    enum {fieldType = FieldPlain};
    FieldType type() const override {return static_cast<FieldType>(fieldType);}
    QVariant schema() const override {
        QVariantMap merged;
        for (const auto &field : this->value.fields()) {
            merged.insert(field, this->value.field(field)->schema());
        }
        QVariantMap actual_schema {
            {"type_name", value.metaObject()->className()},
            {"field_type", fieldType},
            {"field_type_name", QMetaEnum::fromType<FieldType>().valueToKey(fieldType)},
            {"schema", merged},
            {"value", readVariant()}
        };
        return actual_schema;
    }
    NestedIntrospection introspectNested() override {
        return NestedIntrospection(&this->value);
    }
    const NestedIntrospection introspectNested() const override {
        return NestedIntrospection(const_cast<T*>(&this->value));
    }
    QVariant readVariant() const override {
        return this->value.serialize();
    }
    bool updateWithVariant(const QVariant &source) override {
        return this->value.update(source.toMap());
    }
};

template<typename T>
using Field = typename std::conditional<std::is_base_of<Object, T>::value, NestedField<T>, PlainField<T>>::type;


namespace Private {
template<typename T>
struct SequenceCommon : public Private::FieldCommon<QList<T>> {
    using typename Private::FieldCommon<QList<T>>::valueType;
    using typename Private::FieldCommon<QList<T>>::valueRef;
    using Private::FieldCommon<QList<T>>::value;
    using Private::FieldCommon<QList<T>>::FieldCommon;
    using Private::FieldCommon<QList<T>>::operator=;
    typename QList<T>::iterator begin() {
        return value.begin();
    }
    typename QList<T>::iterator end() {
        return value.end();
    }
    typename QList<T>::const_iterator begin() const {
        return value.begin();
    }
    typename QList<T>::const_iterator end() const {
        return value.end();
    }
    typename QList<T>::const_iterator cbegin() const {
        return value.cbegin();
    }
    typename QList<T>::const_iterator cend() const {
        return value.cend();
    }
};
}

template<typename T>
struct PlainSequence : public Private::SequenceCommon<T> {
    using typename Private::SequenceCommon<T>::valueType;
    using typename Private::SequenceCommon<T>::valueRef;
    using Private::SequenceCommon<T>::value;
    using Private::SequenceCommon<T>::SequenceCommon;
    using Private::SequenceCommon<T>::operator=;
    static int staticValueMetaTypeId() {return QMetaType::fromType<T>().id();}
    int valueMetaTypeId() const override {return staticValueMetaTypeId();}
    enum {fieldType = FieldSequence};
    FieldType type() const override {return static_cast<FieldType>(fieldType);}
    QVariant schema() const override {
        QVariantMap actual_schema {
            {"type_name", QMetaType::fromType<QList<T>>().name()},
            {"nested_type_name", QMetaType::fromType<T>().name()},
            {"nested_type_id", QMetaType::fromType<T>().id()},
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
    bool updateWithVariant(const QVariant &source) override {
        QList<T> temp;
        auto asList = source.toList();
        for (auto &src : asList) {
            if (!src.canConvert<T>()) continue;
            auto tempVal = src.value<T>();
            if (src != tempVal) {
                settingsParsingWarn().noquote() << "Types mismatch, while deserializing -->\n Wanted: " <<
                                                   QMetaType::fromType<T>().name() <<
                                                   "\n Actual: " << src;
            } else {
                temp.append(tempVal);
            }
        }
        if (!temp.empty()) {
            this->value = temp;
        }
        return !temp.empty();
    }
};

template<typename T>
struct NestedSequence : public Private::SequenceCommon<T> {
    using typename Private::SequenceCommon<T>::valueType;
    using typename Private::SequenceCommon<T>::valueRef;
    using Private::SequenceCommon<T>::value;
    using Private::SequenceCommon<T>::SequenceCommon;
    using Private::SequenceCommon<T>::operator=;
    static int staticValueMetaTypeId() {return -1;}
    int valueMetaTypeId() const override {return staticValueMetaTypeId();}
    enum {fieldType = FieldSequenceOfNested};
    FieldType type() const override {return static_cast<FieldType>(fieldType);}
    QVariant schema() const override {
        auto nestedSchema = this->value.isEmpty() ? T().schema() : this->value.constFirst().schema();
        auto mobj = this->value.isEmpty() ? T().metaObject() : this->value.constFirst().metaObject();
        QString fullName = QStringLiteral("sequence<%1>").arg(QString(mobj->className()));
        QVariantMap actual_schema {
            {"type_name", fullName},
            {"nested_schema", nestedSchema},
            {"field_type", fieldType},
            {"field_type_name", QMetaEnum::fromType<FieldType>().valueToKey(fieldType)},
            {"value", readVariant()},
        };
        return actual_schema;
    }
    NestedIntrospection introspectNested() override {
        QList<Object*> result;
        for (auto &subval : value) {
            result.append(&subval);
        }
        return NestedIntrospection(result);
    }
    const NestedIntrospection introspectNested() const override {
        QList<Object*> result;
        for (const auto &subval : value) {
            result.append(const_cast<Object*>(static_cast<const Object*>(&subval)));
        }
        return NestedIntrospection(result);
    }
    QVariant readVariant() const override {
        QVariantList values;
        for (const auto &value : this->value) {
            values.append(value.serialize());
        }
        return values;
    }
    bool updateWithVariant(const QVariant &source) override {
        QList<T> temp;
        auto asList = source.toList();
        for (const auto &src : asList) {
            if (!src.canConvert<QVariantMap>()) continue;
            auto current = T();
            auto currentOk = current.update(src.toMap());
            if (currentOk) {
                temp.append(current);
            }
        }
        if (!temp.empty()) {
            this->value = temp;
        }
        return !temp.empty();
    }
};

template<typename T>
using Sequence = typename std::conditional<std::is_base_of<Object, T>::value, NestedSequence<T>, PlainSequence<T>>::type;


namespace Private {
template<typename T>
struct MappingCommon : public Private::FieldCommon<QMap<QString, T>> {
    using typename Private::FieldCommon<QMap<QString, T>>::valueType;
    using typename Private::FieldCommon<QMap<QString, T>>::valueRef;
    using Private::FieldCommon<QMap<QString, T>>::value;
    using Private::FieldCommon<QMap<QString, T>>::FieldCommon;
    using Private::FieldCommon<QMap<QString, T>>::operator=;
    typename QMap<QString, T>::iterator begin() {
        return value.begin();
    }
    typename QMap<QString, T>::iterator end() {
        return value.end();
    }
    typename QMap<QString, T>::const_iterator begin() const {
        return value.begin();
    }
    typename QMap<QString, T>::const_iterator end() const {
        return value.end();
    }
    typename QMap<QString, T>::const_iterator cbegin() const {
        return value.cbegin();
    }
    typename QMap<QString, T>::const_iterator cend() const {
        return value.cend();
    }
};
}

template<typename T>
struct PlainMapping : public Private::MappingCommon<T> {
    using typename Private::MappingCommon<T>::valueType;
    using typename Private::MappingCommon<T>::valueRef;
    using Private::MappingCommon<T>::value;
    using Private::MappingCommon<T>::MappingCommon;
    using Private::MappingCommon<T>::operator=;
    static int staticValueMetaTypeId() {return QMetaType::fromType<T>().id();}
    int valueMetaTypeId() const override {return staticValueMetaTypeId();}
    enum {fieldType = FieldMapping};
    FieldType type() const override {return static_cast<FieldType>(fieldType);}
    QVariant schema() const override {
        QVariantMap actual_schema {
            {"type_name", QMetaType::fromType<QMap<QString, T>>().name()},
            {"nested_type_name", QMetaType::fromType<T>().name()},
            {"nested_type_id", QMetaType::fromType<T>().id()},
            {"field_type", fieldType},
            {"field_type_name", QMetaEnum::fromType<FieldType>().valueToKey(fieldType)},
            {"value", readVariant()},
        };
        return actual_schema;
    }
    QVariant readVariant() const override {
        QVariantMap values;
        for (auto iter = this->value.begin(); iter != this->value.cend(); +iter) {
            values.insert(iter().key(), iter().value().serialize());
        }
        return values;
    }
    bool updateWithVariant(const QVariant &source) override {
        QMap<QString, T> temp;
        auto asMap = source.toMap();
        for (auto iter = asMap.cbegin(); iter != asMap.cend(); ++iter) {
            if (!iter.value().canConvert<T>()) continue;
            auto tempVal = iter->value<T>();
            if (tempVal != iter.value()) {
                settingsParsingWarn().noquote() << "Types mismatch, while deserializing -->\n Wanted: " <<
                                                   QMetaType::fromType<T>().name() <<
                                                   "\n Actual: " << *iter;
            } else {
                temp.insert(iter.key(), tempVal);
            }
        }
        if (!temp.empty()) {
            this->value = temp;
        }
        return !temp.empty();
    }
};

template<typename T>
struct NestedMapping : public Private::MappingCommon<T> {
    using typename Private::MappingCommon<T>::valueType;
    using typename Private::MappingCommon<T>::valueRef;
    using Private::MappingCommon<T>::value;
    using Private::MappingCommon<T>::MappingCommon;
    using Private::MappingCommon<T>::operator=;
    static int staticValueMetaTypeId() {return -1;}
    int valueMetaTypeId() const override {return staticValueMetaTypeId();}
    enum {fieldType = FieldMappingOfNested};
    FieldType type() const override {return static_cast<FieldType>(fieldType);}
    QVariant schema() const override {
        auto nestedSchema = this->value.isEmpty() ? T().schema() : this->value.constFirst().schema();
        auto mobj = this->value.isEmpty() ? T().metaObject() : this->value.constFirst().metaObject();
        QVariantMap actual_schema {
            {"type_name", QMetaType::fromType<QMap<QString, T>>().name()},
            {"nested_type_name", QStringLiteral("mapping<string, %1>").arg(QString(mobj->className()))},
            {"nested_schema", nestedSchema},
            {"field_type", fieldType},
            {"field_type_name", QMetaEnum::fromType<FieldType>().valueToKey(fieldType)},
            {"value", readVariant()},
        };
        return actual_schema;
    }
    QVariant readVariant() const override {
        QVariantMap values;
        for (auto iter = this->value.begin(); iter != this->value.cend(); +iter) {
            values.insert(iter().key() ,iter().value().serialize());
        }
        return values;
    }
    NestedIntrospection introspectNested() override {
        return NestedIntrospection(QMap<QString, Object*>{&this->value.begin(), &this->value.end()});
    }
    const NestedIntrospection introspectNested() const override {
        return NestedIntrospection(QMap<QString, Object*>{&this->value.begin(), &this->value.end()});
    }
    bool updateWithVariant(const QVariant &source) override {
        QMap<QString, T> temp;
        auto asMap = source.toMap();
        for (auto iter = asMap.cbegin(); iter != asMap.cend(); ++iter) {
            if (!iter.value().canConvert<T>()) continue;
            auto current = T();
            auto currentOk = current.update(iter.value().toMap());
            if (currentOk) {
                temp.insert(iter.key(), current);
            }
        }
        if (!temp.empty()) {
            this->value = temp;
        }
        return !temp.empty();
    }
};

template<typename T>
using Mapping = typename std::conditional<std::is_base_of<Object, T>::value, NestedMapping<T>, PlainMapping<T>>::type;

}


#endif // SERIALIZER_H
