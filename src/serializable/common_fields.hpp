#ifndef COMMON_FIELDS_H
#define COMMON_FIELDS_H

#include "private/global.h"
#include "private/impl_serializable.h"
#include "radapterlogging.h"
#include "field_super.h"

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
    struct FieldConcept {
        friend Object;
        virtual QVariantMap schema(const Object *owner) const = 0;
        virtual QVariant readVariant(const Object *owner) const = 0;
        virtual bool updateWithVariant(Object *owner, const QVariant &source) = 0;
        virtual FieldType fieldType(const Object *owner) const = 0;
        virtual int valueMetaTypeId(const Object *owner) const = 0;
        virtual void *rawValue(Object *owner) = 0;
        virtual const void *rawValue(const Object *owner) const = 0;
        virtual const QStringList &attributes(const Object *owner) const = 0;
        virtual const NestedIntrospection introspectNested(const Object *owner) const = 0;
        virtual NestedIntrospection introspectNested(Object *owner) = 0;
        virtual const QString &fieldRepr(const Object *owner) const = 0;
        virtual const QString &typeName(const Object *owner) const = 0;
        bool isNested(const Object *owner) const {
            auto res = fieldType(owner);
            return res == FieldNested
                    || res == FieldSequenceOfNested
                    || res == FieldMappingOfNested;
        }
        bool isMapping(const Object *owner) const {
            auto res = fieldType(owner);
            return res == FieldMapping
                   || res == FieldMappingOfNested;
        }
        bool isSequence(const Object *owner) const {
            auto res = fieldType(owner);
            return res == FieldSequence
                   || res == FieldSequenceOfNested;
        }
        virtual ~FieldConcept() = default;
    };
}
Q_DECLARE_METATYPE(Serializable::FieldConcept*)
Q_DECLARE_METATYPE(QSharedPointer<Serializable::FieldConcept>)
namespace Serializable {
namespace Private {
template <typename Class, typename Field>
struct FieldHolder : FieldConcept {
    FieldHolder(Field Class::*fieldGetter) : m_fieldGetter(fieldGetter) {}
    QVariantMap schema(const Object *owner) const override final {
        auto result = field(owner)->schema();
        result["attributes"] = field(owner)->attributes();
        return result;
    }
    QVariant readVariant(const Object *owner) const override final {
        return field(owner)->readVariant();
    }
    bool updateWithVariant(Object *owner, const QVariant &source) override final {
        return field(owner)->updateWithVariant(source);
    }
    FieldType fieldType(const Object *owner) const override final {
        return field(owner)->fieldType();
    }
    int valueMetaTypeId(const Object *owner) const override final {
        return field(owner)->valueMetaTypeId();
    }
    void *rawValue(Object *owner) override final {
        return field(owner)->rawValue();
    }
    const void *rawValue(const Object *owner) const override final {
        return field(owner)->rawValue();
    }
    const QStringList &attributes(const Object *owner) const override final {
        return field(owner)->attributes();
    }
    NestedIntrospection introspectNested(Object *owner) override final {
        return field(owner)->introspectNested();
    }
    const NestedIntrospection introspectNested(const Object *owner) const override final {
        return field(owner)->introspectNested();
    }
    const QString &fieldRepr(const Object *owner) const override final {
        return field(owner)->fieldRepr();
    }
    const QString &typeName(const Object *owner) const override final {
        return field(owner)->typeName();
    }
protected:
    const Field *field(const Object *owner) const {
        return const_cast<FieldHolder*>(this)->field(const_cast<Object*>(owner));
    }
    Field *field(Object *owner) {
        return &(reinterpret_cast<Class*>(owner)->*m_fieldGetter);
    }
private:
    Field Class::*m_fieldGetter;
};
template <typename Class, typename Field>
QVariant upcastField(Field Class::*fieldGetter) {
    return QVariant::fromValue(QSharedPointer<FieldConcept>(new Private::FieldHolder<Class, Field>(fieldGetter)));
}
struct IsFieldCheck {};
template<typename T>
struct FieldCommon : IsFieldCheck {
    friend Serializable::Object;
    using valueType = T;
    using valueRef = T&;
    FieldCommon() = default;
    template<typename U>
    FieldCommon(U&&other) : value(std::forward<U>(other)) {}
    FieldCommon(const FieldCommon &other) = default;
    template<typename U>
    FieldCommon &operator=(U&&other){value = std::forward<U>(other); return *this;}
    FieldCommon &operator=(const FieldCommon &other) = default;
    bool operator==(const T& other) const {return value == other;}
    bool operator==(const FieldCommon &other) const {return value == other.value;}
    T* operator->() {return &value;}
    const T* operator->() const {return &value;}
    T& operator*() {return value;}
    const T& operator*() const {return value;}
    operator T&() {return value;}
    operator const T&() const {return value;}
    operator QVariant() const {return QVariant(value);}
    const QString &typeName() const {static QString res{QMetaType::fromType<T>().name()}; return res;}
    void *rawValue() {return &value;}
    const QStringList &attributes() const {static QStringList attrs; return attrs;}
    const NestedIntrospection introspectNested() const {return NestedIntrospection();}
    NestedIntrospection introspectNested() {return NestedIntrospection();}
    const void *rawValue() const {return &value;}
    void update(const T& newVal) {this->value = newVal;}
    T value;
};
}

template<typename T>
struct PlainField : public Private::FieldCommon<T>
{
    using typename Private::FieldCommon<T>::valueType;
    using typename Private::FieldCommon<T>::valueRef;
    using Private::FieldCommon<T>::FieldCommon;
    using Private::FieldCommon<T>::operator=;
    using Private::FieldCommon<T>::operator==;
    using Private::FieldCommon<T>::value;
    static int staticValueMetaTypeId() {return QMetaType::fromType<T>().id();}
    const QString &fieldRepr() const {
        static QString res = QString{"Plain<"} + this->typeName() + '>';
        return res;
    }
    int valueMetaTypeId() const {return staticValueMetaTypeId();}
    enum {thisFieldType = FieldPlain};
    FieldType fieldType() const {return static_cast<FieldType>(thisFieldType);}
    QVariantMap schema() const {
        QVariantMap actual_schema {
            {"type_id", QMetaType::fromType<T>().id()},
            {"type_name", QMetaType::fromType<T>().name()},
            {"field_type", thisFieldType},
            {"is_nested", false},
            {"field_type_name", QMetaEnum::fromType<FieldType>().valueToKey(thisFieldType)},
            {"value", readVariant()},
        };
        return actual_schema;
    }
    QVariant readVariant() const {
        return QVariant::fromValue(this->value);
    }
    bool updateWithVariant(const QVariant &source) {
        if (source.canConvert<T>()) {
            this->value = source.value<T>();
            return true;
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
    using Private::FieldCommon<T>::operator==;
    static int staticValueMetaTypeId() {return -1;}
    int valueMetaTypeId() const {return staticValueMetaTypeId();}
    enum {thisFieldType = FieldPlain};
    FieldType fieldType() const {return static_cast<FieldType>(thisFieldType);}
    const QString &typeName() const {
        static QString res{typeid(T).name()};
        return res;
    }
    const QString &fieldRepr() const {
        static QString res = QString{"Nested<"} + this->typeName() + '>';
        return res;
    }
    QVariantMap schema() const {
        QVariantMap merged;
        for (const auto &field : this->value.fields()) {
            merged.insert(field, this->value.field(field)->schema(&this->value));
        }
        QVariantMap actual_schema {
            {"type_name", value.metaObject()->className()},
            {"field_type", thisFieldType},
            {"is_nested", true},
            {"field_type_name", QMetaEnum::fromType<FieldType>().valueToKey(thisFieldType)},
            {"schema", merged},
            {"value", readVariant()}
        };
        return actual_schema;
    }
    NestedIntrospection introspectNested() {
        return NestedIntrospection(&this->value);
    }
    const NestedIntrospection introspectNested() const {
        return NestedIntrospection(const_cast<T*>(&this->value));
    }
    QVariant readVariant() const {
        return this->value.serialize();
    }
    bool updateWithVariant(const QVariant &source) {
        return this->value.update(source.toMap());
    }
};

template<typename T>
using Plain = typename std::conditional<std::is_base_of<Object, T>::value, NestedField<T>, PlainField<T>>::type;

namespace Private {
template<typename T>
struct SequenceCommon : public Private::FieldCommon<QList<T>> {
    using typename Private::FieldCommon<QList<T>>::valueType;
    using typename Private::FieldCommon<QList<T>>::valueRef;
    using Private::FieldCommon<QList<T>>::value;
    using Private::FieldCommon<QList<T>>::FieldCommon;
    using Private::FieldCommon<QList<T>>::operator=;
    using Private::FieldCommon<QList<T>>::operator==;
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
    using Private::SequenceCommon<T>::operator==;
    static int staticValueMetaTypeId() {return QMetaType::fromType<T>().id();}
    int valueMetaTypeId() const {return staticValueMetaTypeId();}
    const QString &fieldRepr() const {
        static QString res = QString{"Sequence<"} + this->typeName() + '>';
        return res;
    }
    enum {thisFieldType = FieldSequence};
    FieldType fieldType() const {return static_cast<FieldType>(thisFieldType);}
    QVariantMap schema() const {
        QVariantMap actual_schema {
            {"type_name", QMetaType::fromType<QList<T>>().name()},
            {"nested_type_name", QMetaType::fromType<T>().name()},
            {"nested_type_id", QMetaType::fromType<T>().id()},
            {"field_type", thisFieldType},
            {"is_nested", false},
            {"field_type_name", QMetaEnum::fromType<FieldType>().valueToKey(thisFieldType)},
            {"value", readVariant()},
        };
        return actual_schema;
    }
    QVariant readVariant() const {
        QVariantList values;
        for (const auto &value : this->value) {
            values.append(QVariant::fromValue(value));
        }
        return values;
    }
    bool updateWithVariant(const QVariant &source) {
        QList<T> temp;
        auto asList = source.toList();
        for (auto &src : asList) {
            if (!src.canConvert<T>()) continue;
            temp.append(src.value<T>());
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
    using Private::SequenceCommon<T>::operator==;
    static int staticValueMetaTypeId() {return -1;}
    const QString &typeName() const {
        static QString res{typeid(T).name()};
        return res;
    }
    const QString &fieldRepr() const {
        static QString res = QString{"Sequence<"} + this->typeName() + '>';
        return res;
    }
    int valueMetaTypeId() const {return staticValueMetaTypeId();}
    enum {thisFieldType = FieldSequenceOfNested};
    FieldType fieldType() const {return static_cast<FieldType>(thisFieldType);}
    QVariantMap schema() const {
        auto nestedSchema = this->value.isEmpty() ? T().schema() : this->value.constFirst().schema();
        auto mobj = this->value.isEmpty() ? T().metaObject() : this->value.constFirst().metaObject();
        QString fullName = QStringLiteral("sequence<%1>").arg(QString(mobj->className()));
        QVariantMap actual_schema {
            {"type_name", fullName},
            {"nested_schema", nestedSchema},
            {"field_type", thisFieldType},
            {"is_nested", true},
            {"field_type_name", QMetaEnum::fromType<FieldType>().valueToKey(thisFieldType)},
            {"value", readVariant()},
        };
        return actual_schema;
    }
    NestedIntrospection introspectNested() {
        QList<Object*> result;
        for (auto &subval : value) {
            result.append(&subval);
        }
        return NestedIntrospection(result);
    }
    const NestedIntrospection introspectNested() const {
        QList<Object*> result; // const correctness is preserved by returning const NestedIntrospection
        for (const auto &subval : value) {
            result.append(const_cast<Object*>(static_cast<const Object*>(&subval)));
        }
        return NestedIntrospection(result);
    }
    QVariant readVariant() const {
        QVariantList values;
        for (const auto &value : this->value) {
            values.append(value.serialize());
        }
        return values;
    }
    bool updateWithVariant(const QVariant &source) {
        QList<T> temp;
        const auto asList = source.toList();
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
    using Private::FieldCommon<QMap<QString, T>>::operator==;
    typename QMap<QString, T>::key_value_iterator begin() {
        return value.keyValueBegin();
    }
    typename QMap<QString, T>::key_value_iterator end() {
        return value.keyValueEnd();
    }
    typename QMap<QString, T>::const_key_value_iterator begin() const {
        return value.keyValueBegin();
    }
    typename QMap<QString, T>::const_key_value_iterator end() const {
        return value.keyValueEnd();
    }
    typename QMap<QString, T>::const_key_value_iterator cbegin() const {
        return value.keyValueBegin();
    }
    typename QMap<QString, T>::const_key_value_iterator cend() const {
        return value.keyValueEnd();
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
    using Private::MappingCommon<T>::operator==;
    static int staticValueMetaTypeId() {return QMetaType::fromType<T>().id();}
    int valueMetaTypeId() const {return staticValueMetaTypeId();}
    const QString &fieldRepr() const {
        static QString res = QString{"Mapping<"} + this->typeName() + '>';
        return res;
    }
    enum {thisFieldType = FieldMapping};
    FieldType fieldType() const {return static_cast<FieldType>(thisFieldType);}
    QVariantMap schema() const {
        QVariantMap actual_schema {
            {"type_name", QMetaType::fromType<QMap<QString, T>>().name()},
            {"nested_type_name", QMetaType::fromType<T>().name()},
            {"nested_type_id", QMetaType::fromType<T>().id()},
            {"field_type", thisFieldType},
            {"is_nested", false},
            {"field_type_name", QMetaEnum::fromType<FieldType>().valueToKey(thisFieldType)},
            {"value", readVariant()},
        };
        return actual_schema;
    }
    QVariant readVariant() const {
        QVariantMap values;
        for (auto iter = this->value.begin(); iter != this->value.cend(); ++iter) {
            values.insert(iter.key(), QVariant::fromValue(iter.value()));
        }
        return values;
    }
    bool updateWithVariant(const QVariant &source) {
        QMap<QString, T> temp;
        auto asMap = source.toMap();
        for (auto iter = asMap.cbegin(); iter != asMap.cend(); ++iter) {
            if (!iter.value().canConvert<T>()) continue;
            auto tempVal = iter->value<T>();
            temp.insert(iter.key(), iter->value<T>());
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
    using Private::MappingCommon<T>::operator==;
    static int staticValueMetaTypeId() {return -1;}
    const QString &typeName() const {
        static QString res{typeid(T).name()};
        return res;
    }
    const QString &fieldRepr() const {
        static QString res = QString{"Mapping<"} + this->typeName() + '>';
        return res;
    }
    int valueMetaTypeId() const {return staticValueMetaTypeId();}
    enum {thisFieldType = FieldMappingOfNested};
    FieldType fieldType() const {return static_cast<FieldType>(thisFieldType);}
    QVariantMap schema() const {
        auto nestedSchema = this->value.isEmpty() ? T().schema() : this->value.first().schema();
        auto mobj = this->value.isEmpty() ? T().metaObject() : this->value.first().metaObject();
        QVariantMap actual_schema {
            {"type_name", QMetaType::fromType<QMap<QString, T>>().name()},
            {"nested_type_name", QStringLiteral("mapping<string, %1>").arg(QString(mobj->className()))},
            {"nested_schema", nestedSchema},
            {"field_type", thisFieldType},
            {"is_nested", true},
            {"field_type_name", QMetaEnum::fromType<FieldType>().valueToKey(thisFieldType)},
            {"value", readVariant()},
        };
        return actual_schema;
    }
    QVariant readVariant() const {
        QVariantMap values;
        for (auto iter = this->value.begin(); iter != this->value.cend(); ++iter) {
            values.insert(iter.key(), iter.value().serialize());
        }
        return values;
    }
    NestedIntrospection introspectNested() {
        QMap<QString, Object*> result;
        for (auto iter = this->value.begin(); iter != this->value.end(); ++iter) {
            result.insert(iter.key(), &iter.value());
        }
        return NestedIntrospection(result);
    }
    const NestedIntrospection introspectNested() const {
        QMap<QString, Object*> result;
        for (auto iter = this->value.begin(); iter != this->value.end(); ++iter) {
            result.insert(iter.key(), const_cast<Object*>((const Object*) &iter.value()));
        }
        return NestedIntrospection(result);
    }
    bool updateWithVariant(const QVariant &source) {
        QMap<QString, T> temp;
        auto asMap = source.toMap();
        for (auto iter = asMap.cbegin(); iter != asMap.cend(); ++iter) {
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
