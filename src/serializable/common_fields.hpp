#ifndef COMMON_FIELDS_H
#define COMMON_FIELDS_H

#include "private/global.h"
#include "private/impl_serializable.h"
#include "radapterlogging.h"
#include "field_super.h"

namespace Serializable {
    struct FieldConcept;
    struct Object;
    struct IsFieldCheck {};
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
        virtual const IsFieldCheck *rawField(const Serializable::Object *owner) const = 0;
        virtual IsFieldCheck *rawField(Serializable::Object *owner) = 0;
        virtual const QVariantMap &schema(const Serializable::Object *owner) const = 0;
        virtual const QVariant &structure(const Serializable::Object *owner) const = 0;
        virtual QVariant readVariant(const Serializable::Object *owner) const = 0;
        virtual bool updateWithVariant(Serializable::Object *owner, const QVariant &source) = 0;
        virtual FieldType fieldType(const Serializable::Object *owner) const = 0;
        virtual int valueMetaTypeId(const Serializable::Object *owner) const = 0;
        virtual void *rawValue(Serializable::Object *owner) = 0;
        virtual Object* constructNested(const Serializable::Object *owner) const = 0;
        virtual const void *rawValue(const Serializable::Object *owner) const = 0;
        virtual const QStringList &attributes(const Serializable::Object *owner) const = 0;
        virtual const NestedIntrospection introspect(const Serializable::Object *owner) const = 0;
        virtual NestedIntrospection introspect(Serializable::Object *owner) = 0;
        virtual const QString &typeName(const Serializable::Object *owner) const = 0;
        virtual const QString &fieldRepr(const Serializable::Object *owner) const = 0;
        bool isNested(const Serializable::Object *owner) const {
            auto res = fieldType(owner);
            return res == FieldNested
                    || res == FieldSequenceOfNested
                    || res == FieldMappingOfNested;
        }
        bool isMapping(const Serializable::Object *owner) const {
            auto res = fieldType(owner);
            return res == FieldMapping
                   || res == FieldMappingOfNested;
        }
        bool isSequence(const Serializable::Object *owner) const {
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
    FieldHolder(Field Class::*fieldGetter) :
            m_fieldGetter(fieldGetter)
        {}
    const IsFieldCheck *rawField(const Serializable::Object *owner) const override final
    {
        return field(owner);
    }
    IsFieldCheck *rawField(Serializable::Object *owner) override final
    {
        return field(owner);
    }
    const QVariantMap &schema(const Serializable::Object *owner) const override final {
        static QVariantMap res = [this, owner] {
            auto result = field(owner)->schema();
            result["__attributes__"] = field(owner)->attributes();
            result["__type_name__"] = typeName(owner);
            result["__field_repr__"] = fieldRepr(owner);
            auto nestedSchema = result.take("__nested_schema__").toMap();
            for (auto iter = nestedSchema.cbegin(); iter != nestedSchema.cend(); ++iter) {
                result.insert(iter.key(), iter.value());
            }
            return result;
        }();
        return res;
    }
    const QVariant &structure(const Serializable::Object *owner) const override final {
        static QVariant res = [this, owner] {
            auto &full = schema(owner);
            QVariantMap result;
            for (auto iter = full.cbegin(); iter != full.cend(); ++iter) {
                if (!iter.key().startsWith(QStringLiteral("__"))) {
                    result.insert(iter.key(), iter.value().toMap().value(QStringLiteral("__type_name__")));
                }
            }
            if (result.isEmpty()) {
                return full.value("__type_name__");
            }
            return QVariant(result);
        }();
        return res;
    }
    QVariant readVariant(const Serializable::Object *owner) const override final {
        return field(owner)->readVariant();
    }
    bool updateWithVariant(Serializable::Object *owner, const QVariant &source) override final {
        return field(owner)->updateWithVariant(source);
    }
    FieldType fieldType(const Serializable::Object *owner) const override final {
        return static_cast<FieldType>(field(owner)->fieldType());
    }
    int valueMetaTypeId(const Serializable::Object *owner) const override final {
        return field(owner)->valueMetaTypeId();
    }
    Object* constructNested(const Serializable::Object *owner) const override final {
        return field(owner)->constructNested();
    }
    void *rawValue(Serializable::Object *owner) override final {
        return field(owner)->rawValue();
    }
    const void *rawValue(const Serializable::Object *owner) const override final {
        return field(owner)->rawValue();
    }
    const QStringList &attributes(const Serializable::Object *owner) const override final {
        return field(owner)->attributes();
    }
    NestedIntrospection introspect(Serializable::Object *owner) override final {
        return field(owner)->introspect();
    }
    const NestedIntrospection introspect(const Serializable::Object *owner) const override final {
        return field(owner)->introspect();
    }
    const QString &typeName(const Serializable::Object *owner) const override final {
        return field(owner)->typeName();
    }
    const QString &fieldRepr(const Serializable::Object *owner) const override final{
        static QString res =
            QMetaEnum::fromType<FieldType>().valueToKey(fieldType(owner)) + QStringLiteral("<")+typeName(owner)+'>';
        return res;
    }
protected:
    const Field *field(const Serializable::Object *owner) const {
        return const_cast<FieldHolder*>(this)->field(const_cast<Serializable::Object*>(owner));
    }
    Field *field(Serializable::Object *owner) {
        return &(reinterpret_cast<Class*>(owner)->*m_fieldGetter);
    }
private:
    Field Class::*m_fieldGetter;
};
template <typename Class, typename Field>
FieldConcept* upcastField(Field Class::*fieldGetter) {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wall"
#endif
    return new Private::FieldHolder<Class, Field>(fieldGetter); // NOLINT
    // only used for init of static Map<Name, FieldConcept*>. Concept lifetime should be static
#ifdef __clang__
#pragma clang diagnostic pop
#endif

}
template<typename T>
struct FieldCommon : IsFieldCheck {
    friend Serializable::Object;
    template <typename Class, typename Field>
    friend struct Private::FieldHolder;
    using valueType = T;
    using valueRef = T&;
    FieldCommon() = default;
    template<typename U>
    explicit FieldCommon(U&&other) : value(std::forward<U>(other)) {}
    FieldCommon(const FieldCommon &other) = default;
    template<typename U>
    FieldCommon &operator=(const U&other) {value = other; return *this;}
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
    void update(const T& newVal) {this->value = newVal;}
    T value;
protected:
    static int valueMetaTypeId() {return QMetaType::fromType<T>().id();}
    const QString &typeName() const {
        static QString res = QMetaType(valueMetaTypeId()).name();
        return res;
    }
    Object* constructNested() const {return nullptr;}
    void *rawValue() {return &value;}
    const QStringList &attributes() const {static QStringList attrs; return attrs;}
    const NestedIntrospection introspect() const {return NestedIntrospection();}
    NestedIntrospection introspect() {return NestedIntrospection();}
    const void *rawValue() const {return &value;}
};
template<typename T>
struct NestedCommon : FieldCommon<T> {
    using typename Private::FieldCommon<T>::valueType;
    using typename Private::FieldCommon<T>::valueRef;
    using Private::FieldCommon<T>::FieldCommon;
    using Private::FieldCommon<T>::operator=;
    using Private::FieldCommon<T>::operator==;
    using Private::FieldCommon<T>::value;
    template <typename Class, typename Field>
    friend struct Private::FieldHolder;
    static int valueMetaTypeId() {
        static auto mobj = T().metaObject();
        static auto id = mobj->metaType().id();
        return id;
    }
    const QString &typeName() const {
        static QString res = QMetaType(valueMetaTypeId()).name();
        return res;
    }
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
    template <typename Class, typename Field>
    friend struct Private::FieldHolder;
protected:
    enum {thisFieldType = FieldPlain};
    FieldType fieldType() const {return static_cast<FieldType>(thisFieldType);}
    QVariantMap schema() const {
        static QVariantMap actual_schema {
            {"__field_type__", thisFieldType},
            {"__is_nested__", false},
            {"__field_type_name__", QMetaEnum::fromType<FieldType>().valueToKey(thisFieldType)},
        };
        return actual_schema;
    }
    QVariant readVariant() const {
        return QVariant::fromValue(this->value);
    }
    bool updateWithVariant(const QVariant &source) {
        if (source.canConvert<T>()) {
            auto copy = source;
            if (!copy.convert(QMetaType::fromType<T>())) {
                settingsParsingWarn() << "Type can be converted, but is invalid (e.g. string to int failed)";
                return false;
            }
            this->value = copy.value<T>();
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
    template <typename Class, typename Field>
    friend struct Private::FieldHolder;
protected:
    Object* constructNested() const {return new T;}
    enum {thisFieldType = FieldNested};
    FieldType fieldType() const {return static_cast<FieldType>(thisFieldType);}
    QVariantMap schema() const {
        static QVariantMap actual_schema = [&]{
            return QVariantMap {
                {"__nested_fields__", this->value.fields()},
                {"__nested_schema__", this->value.schema()},
                {"__field_type__", thisFieldType},
                {"__is_nested__", true},
                {"__field_type_name__", QMetaEnum::fromType<FieldType>().valueToKey(thisFieldType)},
            };
        }();
        return actual_schema;
    }
    NestedIntrospection introspect() {
        return NestedIntrospection(&this->value);
    }
    const NestedIntrospection introspect() const {
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
    using iterator = typename QList<T>::iterator;
    using const_iterator = typename QList<T>::const_iterator;
    template <typename Class, typename Field>
    friend struct Private::FieldHolder;
    template <typename Key>
    decltype(auto) operator[](Key &&key) {
        return value[std::forward<Key>(key)];
    }
    template <typename Key>
    decltype(auto) operator[](Key &&key) const {
        return value[std::forward<Key>(key)];
    }
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
    template <typename Class, typename Field>
    friend struct Private::FieldHolder;
protected:
    enum {thisFieldType = FieldSequence};
    FieldType fieldType() const {return static_cast<FieldType>(thisFieldType);}
    QVariantMap schema() const {
        static QVariantMap actual_schema {
            {"__field_type__", thisFieldType},
            {"__is_nested__", false},
            {"__field_type_name__", QMetaEnum::fromType<FieldType>().valueToKey(thisFieldType)},
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
            auto copy = src;
            if (!copy.convert(QMetaType::fromType<T>())) {
                settingsParsingWarn() << "Type can be converted, but is invalid (e.g. string to int failed)";
                continue;
            }
            temp.append(copy.value<T>());
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
    template <typename Class, typename Field>
    friend struct Private::FieldHolder;
protected:
    Object* constructNested() const {return new T;}
    static int valueMetaTypeId() {
        static auto mobj = T().metaObject();
        static auto id = mobj->metaType().id();
        return id;
    }
    enum {thisFieldType = FieldSequenceOfNested};
    FieldType fieldType() const {return static_cast<FieldType>(thisFieldType);}
    QVariantMap schema() const {
        static QVariantMap actual_schema = [&]{
            const auto &nested = this->value.isEmpty() ? T() : this->value.constFirst();
            return QVariantMap {
                {"__nested_fields__", nested.fields()},
                {"__nested_schema__", nested.schema()},
                {"__field_type__", thisFieldType},
                {"__is_nested__", true},
                {"__field_type_name__", QMetaEnum::fromType<FieldType>().valueToKey(thisFieldType)},
            };
        }();
        return actual_schema;
    }
    NestedIntrospection introspect() {
        QList<Object*> result;
        for (auto &subval : value) {
            result.append(&subval);
        }
        return NestedIntrospection(result);
    }
    const NestedIntrospection introspect() const {
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
    template <typename Class, typename Field>
    friend struct Private::FieldHolder;
    template <typename Key>
    decltype(auto) operator[](Key &&key) {
        return value[std::forward<Key>(key)];
    }
    template <typename Key>
    decltype(auto) operator[](Key &&key) const {
        return value[std::forward<Key>(key)];
    }
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
    template <typename Class, typename Field>
    friend struct Private::FieldHolder;
protected:
    enum {thisFieldType = FieldMapping};
    FieldType fieldType() const {return static_cast<FieldType>(thisFieldType);}
    QVariantMap schema() const {
        static QVariantMap actual_schema {
            {"__field_type__", thisFieldType},
            {"__is_nested__", false},
            {"__field_type_name__", QMetaEnum::fromType<FieldType>().valueToKey(thisFieldType)}
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
            auto copy = iter.value();
            if (!copy.convert(QMetaType::fromType<T>())) {
                settingsParsingWarn() << "Type can be converted, but is invalid (example: string to int failed)";
                continue;
            }
            temp.insert(iter.key(), copy.value<T>());
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
    template <typename Class, typename Field>
    friend struct Private::FieldHolder;
protected:
    Object* constructNested() const {return new T;}
    static int valueMetaTypeId() {
        static auto mobj = T().metaObject();
        static auto id = mobj->metaType().id();
        return id;
    }
    enum {thisFieldType = FieldMappingOfNested};
    FieldType fieldType() const {return static_cast<FieldType>(thisFieldType);}
    QVariantMap schema() const {
        static QVariantMap actual_schema = [&]{
            const auto &nested = this->value.isEmpty() ? T() : this->value.first();
            return QVariantMap {
               {"__nested_fields__", nested.fields()},
               {"__nested_schema__", nested.schema()},
               {"__field_type__", thisFieldType},
               {"__is_nested__", true},
               {"__field_type_name__", QMetaEnum::fromType<FieldType>().valueToKey(thisFieldType)},
            };
        }();
        return actual_schema;
    }
    QVariant readVariant() const {
        QVariantMap values;
        for (auto iter = this->value.begin(); iter != this->value.cend(); ++iter) {
            values.insert(iter.key(), iter.value().serialize());
        }
        return values;
    }
    NestedIntrospection introspect() {
        QMap<QString, Object*> result;
        for (auto iter = this->value.begin(); iter != this->value.end(); ++iter) {
            result.insert(iter.key(), &iter.value());
        }
        return NestedIntrospection(result);
    }
    const NestedIntrospection introspect() const {
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
