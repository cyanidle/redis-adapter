#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <QVariant>
#include <QMetaProperty>
#include <QMetaObject>
#include <type_traits>
#include <stdexcept>
#include "templates/metaprogramming.hpp"
#include "private/global.h"
#include "radapterlogging.h"
namespace Serializer {

struct Serializable
{
public:
    enum FieldType {
        None = 0,
        Field,
        Nested,
        Container,
        ContainerOfNested,
        Map,
        MapOfNested
    };
    virtual void deserialize(const QVariantMap &src);
    virtual QVariantMap serializeField(const QString &fieldName) const;
    virtual QVariantMap serialize() const;
    virtual const QStringList& fields() const = 0;
    virtual const QVariantMap& structure() const = 0;
    virtual QVariant fieldValue(const QString &fieldName) const;
    bool hasField(const QString &fieldName) const;
    const Serializable *getNested(const QString &fieldName) const;
    Serializable *getNested(const QString &fieldName);
    QList<const Serializable*> getNestedInContainer(const QString &fieldName) const;
    QList<Serializable*> getNestedInContainer(const QString &fieldName);
    QMap<QString, const Serializable*> getNestedInMap(const QString &fieldName) const;
    QMap<QString, Serializable*> getNestedInMap(const QString &fieldName);
    bool isContainer(const QString &fieldName) const;
    bool isMap(const QString &fieldName) const;
    FieldType fieldType(const QString &fieldName) const;
    bool wasUpdated(const QString &fieldName) const;
    Serializable *parent();
    const Serializable *parent() const;
    template <class Target> bool is() const;
    template <class Target> Target *as();
    template <class Target> const Target *as() const;
protected:
    void setParent(Serializable *parent);
    QMetaProperty getProperty(const QString &fieldName) const;
    bool isPropertyFromMacro(const std::string &name) const;
    const QSet<quint32> &deserialisedFieldsHash() const;
    void markUpdated(const QString &fieldName);
    bool deserializeOk(const QString &fieldName) const;
    bool fieldHasDefault(const QString &name) const;
    QVariantMap structureUncached() const;
    virtual ~Serializable() = default;
private:
    void throwIfNotExists(const QString &fieldName) const;
    virtual QVariant readProp(const QMetaProperty &prop) const = 0;
    virtual bool writeProp(const QMetaProperty &prop, const QVariant &val) = 0;
    virtual const QMetaObject * metaObject() const = 0;
    virtual void _settings_impl_PostInit(){};
    QSet<quint32> m_deserialized{};
    Serializable *m_parent{};
};


template <class Target>
struct GadgetMixin : public Target
{
public:
    template <typename...Forward>
    GadgetMixin(Forward...args) :
        Target(args...)
    {}
protected:
    QVariant readProp(const QMetaProperty &prop) const override final {
        return prop.readOnGadget(this);
    }
    bool writeProp(const QMetaProperty &prop, const QVariant &val) override final {
        return prop.writeOnGadget(this, val);
    }
};

template <class Target>
struct QObjectMixin : public QObject, public Target
{
public:
    template <typename...Forward>
    QObjectMixin(QObject *parent, Forward...args) :
        QObject(parent),
        Target(args...)
    {}
protected:
    QVariant readProp(const QMetaProperty &prop) const override final {
        return prop.read(this);
    }
    bool writeProp(const QMetaProperty &prop, const QVariant &val) override final {
        return prop.write(this, val);
    }
};

using SerializableGadget = GadgetMixin<Serializable>;
using SerializableQObject = QObjectMixin<Serializable>;

template<class T>   T fromQMap(const QVariantMap &src);
template<class T>   QList<T> fromQList(const QVariantList &src);
template<class T>   QMap<QString, T> convertQMap(const QVariantMap &src);
template<class T>   QList<T> convertQList(const QVariantList &src);
template<class T>   using QStringMap = QMap<QString, T>;

}
Q_DECLARE_METATYPE(Serializer::Serializable*)
#define COMMA ,
#define DEFAULT
#define NO_READ [](){return QVariant();}
#define NO_ACTION [](){;}
#define MAP_MARKER QStringLiteral("<map>")
#define LIST_MARKER QStringLiteral("<list>")
#define _NO_ARGS
#define _BASE_MACRO(type, name, decay, concept, concept_args, props, funcs, oninit, set_args, read_args, map_args, vargs_str, ...) \
public:\
    type name = {__VA_ARGS__}; \
    funcs \
    protected: \
    static type \
    name##_Default() \
{return type{__VA_ARGS__};} \
    constexpr static bool \
    name##_hasDefault()\
{return !QLatin1String(vargs_str).isEmpty();} \
private: \
    friend concept<type, decay, concept_args>; \
    friend Serializer::Priv::Implementations; \
    props \
    Q_PROPERTY(QVariant name READ read_##name WRITE write_##name) \
    Q_PROPERTY(bool name##_hasDefault READ name##_hasDefault) \
    Q_PROPERTY(QVariantMap name##_getMap READ name##_getMap) \
    Q_PROPERTY(QVariant name##_nestedPtr READ name##_nestedPtr) \
    QVariantMap name##_getMap() const {_check_if_has_is_serializable(); \
        return concept<type, decay, concept_args>::get_map(name, QStringLiteral(#name), map_args);}\
    QVariant name##_nestedPtr () { \
        return concept<type, decay, concept_args>::get_ptr(name); \
    } \
    void write_##name(const QVariant &v) { \
        if (concept<type, decay, concept_args>::set_val(this, name, v, set_args)) { \
            this->markUpdated(QStringLiteral(#name)); \
            oninit \
        }; \
    } \
    QVariant read_##name() const { \
        return concept<type, decay, concept_args>::get_val(name, read_args);} \
public:

#define SERIAL_CUSTOM(type, name, setter, getter, ...) \
public: \
    type name = {__VA_ARGS__}; \
    protected: \
    static type name##_Default() {return type{__VA_ARGS__};} \
    constexpr static bool name##_hasDefault() {return !QLatin1String(#__VA_ARGS__).isEmpty();} \
    private: \
    Q_PROPERTY(QVariant name READ _read_##name WRITE write_##name) \
    Q_PROPERTY(bool name##_hasDefault READ name##_hasDefault) \
    Q_PROPERTY(QVariantMap name##_getMap READ name##_getMap) \
    Q_PROPERTY(QVariant name##_nestedPtr READ name##_nestedPtr) \
    QVariantMap name##_getMap () const { \
        _check_if_has_is_serializable(); \
        return {{#name, -1}}; \
    } \
    QVariant name##_nestedPtr () { \
        return QVariant::fromValue \
            (static_cast<void*>(std::is_base_of<::Serializer::Serializable, decltype(name)>() ? &name : nullptr)); \
    } \
    QVariant _read_##name () const { \
        return QVariant(getter()); \
    } \
    void write_##name (const QVariant & newVal) { \
        if (setter(newVal)) { \
            markUpdated(#name); \
    } \
} \
public:

namespace Serializer {
namespace Priv {

template <typename T>
struct is_nested :
    std::conditional<
        std::is_base_of<Serializable, T>::value,
        std::true_type,
        std::false_type
        >::type
{};
template <typename T, typename Ret = void>
using nested_enable_t = typename std::enable_if<is_nested<T>::value, Ret>::type;
template <typename T, typename Ret = void>
using nested_disable_t = typename std::enable_if<is_nested<T>::value, Ret>::type;
template <typename T>
using decay_t = typename std::decay<T>::type;
template <typename T>
using cont_value_t = typename T::iterator::value_type;

struct Implementations {
    template <typename T, class = void>
                          struct is_shared_ptr : std::false_type{};
    template <typename T> struct is_shared_ptr<T, std::void_t<Radapter::enable_if_ptr_typedef<T>>> : std::true_type{};

    template <typename T, typename Ret = void>
    using is_nested_t = typename std::enable_if<is_nested<T>::value, Ret>::type;
    template <typename T, typename Ret = void>
    using not_nested_t = typename std::enable_if<!is_nested<T>::value && !std::is_enum<T>::value, Ret>::type;
    template <typename T, typename Ret = void>
    using is_enum_t = typename std::enable_if<std::is_enum<T>::value, Ret>::type;
    template <typename T, typename Ret = void>
    using is_shared_ptr_t = typename std::enable_if<is_shared_ptr<T>::value &&
                                                    is_nested<typename T::Type>::value, Ret>::type;

    template<typename Wrapped, typename Decay>
    static QVariant map(const Decay& val, is_nested_t<Decay, int>* = 0) {
        return val.structure();
    }
    template<typename Wrapped, typename Decay>
    static QVariant map(const Decay& val, not_nested_t<Decay, int>* = 0) {
        Q_UNUSED(val)
        return QMetaType::fromType<Decay>().id();
    }
    template<typename Wrapped, typename Decay>
    static QVariant map(const Decay& val, is_enum_t<Decay, int>* = 0) {
        Q_UNUSED(val)
        return QMetaType::fromType<Decay>().id();
    }
    template<typename Wrapped, typename Decay>
    static QVariant map(const Wrapped& val, is_shared_ptr_t<Wrapped, int>* = 0) {
        Q_UNUSED(val)
        return QMetaType::fromType<Decay>().id();
    }

    template<typename Wrapped, typename Decay>
    static Serializable* get_ptr(Decay& val, is_nested_t<Decay, int>* = 0) {
        Q_UNUSED(val)
        return &val;
    }
    template<typename Wrapped, typename Decay>
    static Serializable* get_ptr(Decay& val, is_enum_t<Decay, int>* = 0) {
        Q_UNUSED(val)
        return nullptr;
    }
    template<typename Wrapped, typename Decay>
    static Serializable* get_ptr(Wrapped& val, is_shared_ptr_t<Wrapped, int>* = 0) {
        Q_UNUSED(val)
        return get_ptr<Wrapped, Decay>(*val);
    }
    template<typename Wrapped, typename Decay>
    static Serializable* get_ptr(Decay& val, not_nested_t<Decay, int>* = 0) {
        Q_UNUSED(val)
        return nullptr;
    }

    template<typename Wrapped, typename Decay>
    static QVariant val(const Decay& val, is_nested_t<Decay, int>* = 0) {
        return val.serialize();
    }
    template<typename Wrapped, typename Decay, typename Map>
    static QVariant val(const Decay& val, const Map &mapper) {
        return mapper.key(val);
    }
    template<typename Wrapped, typename Decay>
    static QVariant val(const Wrapped& val, is_shared_ptr_t<Wrapped, int>* = 0) {
        if (val) return Implementations::val<Decay>(*val);
        else return {};
    }
    template<typename Wrapped, typename Decay>
    static QVariant val(const Decay& val, not_nested_t<Decay, int>* = 0) {
        return QVariant::fromValue(val);
    }
    template<typename Wrapped, typename Decay>
    static QVariant val(const Decay& val, is_enum_t<Decay, int>* = 0) {
        return QVariant::fromValue(val);
    }
    template<typename Wrapped, typename Decay, typename Map>
    static bool /* Mapped extraction */
    set(Decay& val, const QVariant &newVal, Serializable *parent, const Map &mapper) {
        Q_UNUSED(parent);
        static_assert(Radapter::ContainerInfo<Map>::has_contains_key, "Map must provide .key(value) .value(key) .contains(value) methods!");
        auto keyVal = newVal.value<Radapter::container_key_t<Map>>();
        if (mapper.contains(keyVal)) {
            val = mapper.value(keyVal);
            return true;
        }
        return false;
    }
    template<typename Wrapped, typename Decay>
    static bool /* Nested extraction */
    set(Decay& val, const QVariant &newVal, Serializable *parent, is_nested_t<Decay, int>* = 0) {
        val.setParent(parent);
        val.deserialize(newVal.toMap());
        return val.deserialisedFieldsHash().size();
    }
    template<typename Wrapped, typename Decay>
    static bool /* Simple extraction */
    set(Decay& val, const QVariant &newVal, Serializable *parent, not_nested_t<Decay, int>* = 0) {
        Q_UNUSED(parent);
        static_assert(QMetaTypeId2<Decay>::Defined,
                "Underlying type must be registered with Q_DECLARE_METATYPE!");
        if (std::is_same<Decay, QVariant>() || newVal.canConvert<Decay>()) {
            auto actualValue = newVal.value<Decay>();
            if (actualValue != newVal) {
                settingsParsingWarn().noquote() << "Types mismatch, while deserializing -->\n Wanted: " <<
                                                   QMetaType::fromType<Decay>().name() <<
                                                   "\n Actual: " << newVal;
            }
            val = std::move(actualValue);
            return true;
        }
        return false;
    }
    template<typename Wrapped, typename Decay>
    static bool /* Simple extraction */
    set(Decay& val, const QVariant &newVal, Serializable *parent, is_enum_t<Decay, int>* = 0) {
        Q_UNUSED(parent);
        static_assert(QMetaTypeId2<Decay>::Defined,
                "Underlying type must be registered with Q_DECLARE_METATYPE!");
        if (std::is_same<Decay, QVariant>() || newVal.canConvert<Decay>()) {
            val = newVal.value<Decay>();
            return true;
        }
        return true;
    }
    template<typename Wrapped, typename Decay>
    static bool /* Pointer extraction */
    set(Wrapped& val, const QVariant &newVal, Serializable *parent, is_shared_ptr_t<Wrapped, int>* = 0) {
        if (!val) val.reset(new Decay{});
        auto status = set<Wrapped, Decay>(*val, newVal, parent);
        if (!status) val.reset();
        return status;
    }
};

//! Concepts implement basic logic of iterating/parsing/calling of Implementations overloads (which handle different underlying types)
/// Must implement set_val, get_val, get_map, can be overloaded based on additional args (set_args, get_args, map_args)
template<typename T, typename Decay, typename... Extra>
struct FieldConcept {
    static_assert(QMetaTypeId2<Decay>::Defined || is_nested<Decay>(),
            "Underlying type must be registered with Q_DECLARE_METATYPE!");
    static QVariantMap get_map(const T& val, QString &&name, int) {
        return {{name, Implementations::map<T, Decay>(val)}};
    };
    static QVariant get_val(const T& val, int) {
        return Implementations::val<T, Decay>(val);
    }
    static QVariant get_ptr(T& val) {
        return QVariant::fromValue(Implementations::get_ptr<T, Decay>(val));
    }
    static bool set_val(Serializable *parent, T& val, const QVariant& newVal, int) {
        if (Implementations::set<T, Decay>(val, newVal, parent)) {
            return true;
        }
        return false;
    }
    //! For automatic conversions from source QVariant to Some Other Type (Like from string in toml to expected Enum)
    /// Used in SERIAL_FIELD_MAPPED
    template<typename Mapping>
    static typename std::enable_if<Radapter::ContainerInfo<Mapping>::has_key &&
                                   Radapter::ContainerInfo<Mapping>::has_value &&
                                   !std::is_same<Mapping, int>::value, bool>::type
    set_val(Serializable *parent, T& val, const QVariant& newVal, const Mapping &mapper) {
        if (Implementations::set<T, Decay, Mapping>(val, newVal, parent, mapper)) {
            return true;
        }
        return false;
    }
    template<typename Mapping>
    static typename std::enable_if<Radapter::ContainerInfo<Mapping>::has_key &&
                                   Radapter::ContainerInfo<Mapping>::has_value &&
                                   !std::is_same<Mapping, int>::value,
                                   QVariant>::type
    get_val(const T& val, const Mapping &mapper) {
        return Implementations::val<T, Decay, Mapping>(val, mapper);
    }
};

template<typename T, typename Decay, typename... Extra>
struct ContainerConcept {
    static_assert(QMetaTypeId2<Decay>::Defined,
            "Underlying type must be registered with Q_DECLARE_METATYPE!");
    static_assert(!std::is_same<
                  decltype(*(std::declval<T>().begin())),
                  const typename T::iterator::value_type&
                >(),
            "Container must provide access to modifiable elements!");
    static QVariantMap get_map(const T& val, QString &&name, int) {
        if (val.size()) {
            return {{name,
                     QVariantMap{{LIST_MARKER,
                                        Implementations::map<cont_value_t<T>, Decay>(*val.cbegin())}}}};
        }
        else return {{name,
                     QVariantMap{{LIST_MARKER,
                                        Implementations::map<cont_value_t<T>, Decay>(Decay{})}}}};
    };
    static QVariant get_val(const T& val, int) {
        QVariantList result;
        for (auto &item : val) {
            result.append(Implementations::val<cont_value_t<T>, Decay>(item));
        }
        return result;
    }
    static QVariant get_ptr(T& val) {
        QList<Serializable*> result;
        for (auto subval = val.begin(); subval != val.end(); subval++) {
            auto current = Implementations::get_ptr<T, Decay>(*subval);
            if (!current) return {};
            result.append(current);
        }
        return QVariant::fromValue(result);
    }
    template <typename Container, typename ValueT>
    static typename std::enable_if<Radapter::ContainerInfo<Container>::has_push_back>::type
    append(Container &holder, const ValueT &val) {
        holder.push_back(val);
    }
    template <typename Container, typename ValueT>
    static typename std::enable_if<Radapter::ContainerInfo<Container>::has_insert_one_arg>::type
    append(Container &holder, const ValueT &val) {
        holder.insert(val);
    }
    static bool set_val(Serializable *parent, T& val, const QVariant& newVal, int) {
        if (!newVal.canConvert<QVariantList>()) return false;
        auto source = newVal.toList();
        val.clear();
        val.reserve(source.size());
        bool updated = false;
        for (auto &nested : source) {
            cont_value_t<T> current{};
            if (Implementations::set<cont_value_t<T>, Decay>(current, nested, parent)) {
                updated = true;
            }
            append(val, current);
        }
        return updated;
    }
};

template<typename T, typename Decay, typename... Extra>
struct MapConcept  {
    static_assert(QMetaTypeId2<Decay>::Defined,
            "Underlying type must be registered with Q_DECLARE_METATYPE!");
    static_assert(!std::is_same<
                  decltype(*(std::declval<T>().begin())),
                  const typename T::iterator::value_type&
                >(),
            "Container must provide access to modifiable elements!");
    static QVariantMap get_map(const T& val, QString &&name, int) {
        if (val.size()) {
            return {{name, QVariantMap{{MAP_MARKER,
                                        Implementations::map<Decay>(*val.cbegin())}}}};
        }
        else return {{name, QVariantMap{{MAP_MARKER,
                                        Implementations::map<Decay>(Decay{})}}}};
    };
    static QVariant get_val(const T& val, int) {
        QVariantMap result;
        for (auto iter = val.constBegin(); iter != val.constEnd(); ++iter) {
            result[iter.key()] = std::move(Implementations::val<cont_value_t<T>, Decay>(iter.value()));
        }
        return result;
    }
    static QVariant get_ptr(T& val) {
        QMap<QString, Serializable*> result;
        for (auto iter{val.begin()}; iter != val.end(); ++iter) {
            auto current = Implementations::get_ptr<T, Decay>(*iter);
            if (!current) return {};
            result.insert(iter.key(), current);
        }
        return QVariant::fromValue(result);
    }
    static bool set_val(T& val, const QVariant& newVal, int) {
        if (!newVal.canConvert<QVariantMap>()) return false;
        val.clear();
        bool updated = false;
        auto currentMap = newVal.toMap();
        for (auto iter = currentMap.constBegin(); iter != currentMap.constEnd(); ++iter) {
            if (Implementations::set<cont_value_t<T>, Decay>(val[iter.key()], iter.value())) {
                updated = true;
            }
        }
        return updated;
    }
};
}
}

#define SERIAL_FIELD(type, name, ...) \
_BASE_MACRO(type, name, type/*decay*/, \
            Serializer::Priv::FieldConcept, void/*impl_args*/, \
            _NO_ARGS/*props*/, \
            _NO_ARGS/*funcs*/, \
            _NO_ARGS/*on_init*/, \
            0/*set_args*/, 0/*get_args*/, 0/*map_args*/, #__VA_ARGS__, __VA_ARGS__)

#define SERIAL_FIELD_MAPPED(type, name, map, ...) \
_BASE_MACRO(type, name, type/*decay*/, \
            Serializer::Priv::FieldConcept, void/*impl_args*/, \
            _NO_ARGS/*props*/, \
            _NO_ARGS/*funcs*/, \
            _NO_ARGS/*on_init*/, \
            map/*set_args*/, map/*get_args*/, 0/*map_args*/, #__VA_ARGS__, __VA_ARGS__)

#define SERIAL_FIELD_PTR(type, name, ...) \
_BASE_MACRO(QSharedPointer<type>, name, type/*decay*/, \
            Serializer::Priv::FieldConcept, void/*impl_args*/, \
            _NO_ARGS/*props*/, \
            _NO_ARGS/*funcs*/, \
            _NO_ARGS/*on_init*/, \
            0/*set_args*/, 0/*get_args*/, 0/*map_args*/, #__VA_ARGS__, __VA_ARGS__)

#define SERIAL_FIELD_NOTIFY(type, name, notify, ...) \
_BASE_MACRO(type, name, type/*decay*/, \
            Serializer::Priv::FieldConcept, void/*impl_args*/, \
            _NO_ARGS/*props*/, \
            _NO_ARGS/*funcs*/, \
            _check_is_serializable_qobject(); notify(name);/*on_init*/, \
            0/*set_args*/, 0/*get_args*/, 0/*map_args*/, #__VA_ARGS__, __VA_ARGS__)

#define SERIAL_CONTAINER(container, type, name, ...) \
_BASE_MACRO(container<type>, name, type/*decay*/, \
            Serializer::Priv::ContainerConcept, void/*impl_args*/, \
            _NO_ARGS/*props*/, \
            _NO_ARGS/*funcs*/, \
            _NO_ARGS/*on_init*/, \
            0/*set_args*/, 0/*get_args*/, 0/*map_args*/, #__VA_ARGS__, __VA_ARGS__)

#define SERIAL_CONTAINER_PTRS(container, type, name, ...) \
_BASE_MACRO(container<QSharedPointer<type>>, name, type/*decay*/, \
            Serializer::Priv::ContainerConcept, void/*impl_args*/, \
            _NO_ARGS/*props*/, \
            _NO_ARGS/*funcs*/, \
            _NO_ARGS/*on_init*/, \
            0/*set_args*/, 0/*get_args*/, 0/*map_args*/, #__VA_ARGS__, __VA_ARGS__)

#define SERIAL_CONTAINER_NOTIFY(container, type, name, notify, ...) \
_BASE_MACRO(container<type>, name, type/*decay*/, \
            Serializer::Priv::ContainerConcept, void/*impl_args*/, \
            _NO_ARGS/*props*/, \
            _NO_ARGS/*funcs*/, \
            notify(name);/*on_init*/, \
            0/*set_args*/, 0/*get_args*/, 0/*map_args*/, #__VA_ARGS__, __VA_ARGS__)

#define SERIAL_MAP(type, name, ...) \
_BASE_MACRO(Serializer::QStringMap<type>, name, type/*decay*/, \
            Serializer::Priv::MapConcept, void/*impl_args*/, \
            _NO_ARGS/*props*/, \
            _NO_ARGS/*funcs*/, \
            _NO_ARGS/*on_init*/, \
            0/*set_args*/, 0/*get_args*/, 0/*map_args*/, #__VA_ARGS__, __VA_ARGS__)

#define SERIAL_MAP_NOTIFY(type, name, notify, ...) \
_BASE_MACRO(Serializer::QStringMap<type>, name, type/*decay*/, \
            Serializer::Priv::MapConcept, void/*impl_args*/, \
            _NO_ARGS/*props*/, \
            _NO_ARGS/*funcs*/, \
            notify(name)/*on_init*/, \
            0/*set_args*/, 0/*get_args*/, 0/*map_args*/, #__VA_ARGS__, __VA_ARGS__)

#define SERIAL_POST_INIT(func, ...) \
private: \
    void _settings_impl_PostInit() override { \
        func(__VA_ARGS__); \
} \
public:


#define IS_SERIALIZABLE \
private: \
    void _check_if_has_is_serializable() const noexcept { \
        using type = typename std::decay<decltype(*this)>::type; \
        static_assert(!std::is_base_of<QObject, type>(), \
                      "Must Not Inherit QObject!"); \
        static_assert(!std::is_base_of<QObject, type>(), \
                      "User IS_SERIALIZABLE_QOBJECT for QObjects!"); \
        static_assert(Radapter::Private::has_QGadget_Macro<type>::Value, \
                      "Must have Q_GADGET macro!"); \
} \
    virtual const QMetaObject * metaObject() const override  { \
        return &(this->staticMetaObject); \
} \
public: \
virtual const QStringList& fields() const override { \
    static QStringList fields{structure().keys()}; \
    return fields; \
} \
virtual const QVariantMap& structure() const override { \
    static QVariantMap structure{structureUncached()}; \
    return structure; \
}

// I <3 pani Belska

#define IS_SERIALIZABLE_QOBJECT \
private: \
    void _check_if_has_is_serializable() const noexcept { \
        using type = typename std::decay<decltype(*this)>::type; \
        static_assert(std::is_base_of<QObject, type>(), \
                      "Must Inherit QObject!"); \
        static_assert(Radapter::Private::has_QObject_Macro<type>::Value, \
                      "Must have Q_OBJECT macro!"); \
} \
    void _check_is_serializable_qobject() const noexcept {} \
public: \
virtual const QStringList& fields() const override { \
    static QStringList fields {structure().keys()}; \
    return fields; \
} \
virtual const QVariantMap& structure() const override { \
    static QVariantMap structure {structureUncached()}; \
    return structure; \
}

#define BINARY_SUPPORT(target) \
friend QDataStream &operator << (QDataStream &out, const target &d) \
{ \
    return out << d.serialize(); \
} \
friend QDataStream &operator >> (QDataStream &in, target &d) \
{ \
    QVariantMap json; \
    in >> json; \
    d.deserialize(json, false); \
    return in; \
}

namespace Serializer {

inline void Serializable::deserialize(const QVariantMap &src) {
    m_deserialized.clear();
    for (auto &field : fields()) {
        if (src.contains(field)) {
            if (isPropertyFromMacro(field.toStdString())) {
                this->writeProp(getProperty(field), src[field]);
            }
        }
    }
    _settings_impl_PostInit();
}

inline QVariantMap Serializable::serialize() const {
    auto result = QVariantMap();
    for (auto &field: fields()) {
        auto propIndex = metaObject()->indexOfProperty(field.toStdString().c_str());
        auto prop = metaObject()->property(propIndex);
        result.insert(field, this->readProp(prop));
    }
    return result;
}

inline QVariantMap Serializable::serializeField(const QString &fieldName) const
{
    return {{fieldName, this->readProp(getProperty(fieldName))}};
}

inline QVariantMap Serializable::structureUncached() const {
    QVariantMap result;
    int propCount = metaObject()->propertyCount();
    for(int i = 0; i < propCount; i++) {
        auto propName = std::string(metaObject()->property(i).name());
        if (isPropertyFromMacro(propName)) {
            auto prop = metaObject()->property(metaObject()->indexOfProperty((propName + "_getMap").c_str()));
            result.insert(this->readProp(prop).toMap());
        }
    }
    return result;
}

inline bool Serializable::hasField(const QString &fieldName) const
{
    return fields().contains(fieldName);
}

inline void Serializable::throwIfNotExists(const QString &fieldName) const
{
    if (!fields().contains(fieldName)) {
        throw std::invalid_argument("Field: (" + fieldName.toStdString() + ") --> is not marked SERIAL_<MACRO> (or does not exist)!");
    }
}

inline bool Serializable::isPropertyFromMacro(const std::string &name) const
{
    auto defaultName = name + "_hasDefault";
    auto getMapName = name + "_getMap";
    return metaObject()->indexOfProperty(defaultName.c_str()) != -1 &&
           metaObject()->indexOfProperty(getMapName.c_str()) != -1;
}

inline const QSet<quint32> &Serializable::deserialisedFieldsHash() const
{
    return m_deserialized;
}

inline QVariant Serializable::fieldValue(const QString &fieldName) const
{
    throwIfNotExists(fieldName);
    auto prop = getProperty(fieldName);
    return this->readProp(prop);
}

inline const Serializable *Serializable::getNested(const QString &fieldName) const
{
    return const_cast<Serializable*>(this)->getNested(fieldName);
}

inline Serializable *Serializable::getNested(const QString &fieldName)
{
    throwIfNotExists(fieldName);
    auto prop = getProperty(fieldName + QStringLiteral("_nestedPtr"));
    return this->readProp(prop).value<Serializable*>();
}

inline QList<const Serializable *> Serializable::getNestedInContainer(const QString &fieldName) const
{
    throwIfNotExists(fieldName);
    auto nonConst = const_cast<Serializable*>(this)->getNestedInContainer(fieldName);
    return {nonConst.begin(), nonConst.end()};
}

inline QList<Serializable *> Serializable::getNestedInContainer(const QString &fieldName)
{
    throwIfNotExists(fieldName);
    auto prop = getProperty(fieldName + QStringLiteral("_nestedPtr"));
    return this->readProp(prop).value<QList<Serializable *>>();
}

inline QMap<QString, const Serializable *> Serializable::getNestedInMap(const QString &fieldName) const
{
    throwIfNotExists(fieldName);
    auto nonConst = const_cast<Serializable*>(this)->getNestedInMap(fieldName);
    QMap<QString, const Serializable *> result;
    for (auto iter{nonConst.cbegin()}; iter != nonConst.cend(); ++iter) {
        result.insert(iter.key(), iter.value());
    }
    return result;
}

inline QMap<QString, Serializable *> Serializable::getNestedInMap(const QString &fieldName)
{
    throwIfNotExists(fieldName);
    auto prop = getProperty(fieldName + QStringLiteral("_nestedPtr"));
    return this->readProp(prop).value<QMap<QString, Serializable *>>();

}

inline bool Serializable::isContainer(const QString &fieldName) const
{
    auto type = fieldType(fieldName);
    return type == Container || type == ContainerOfNested;
}

inline bool Serializable::isMap(const QString &fieldName) const
{
    auto type = fieldType(fieldName);
    return type == Map || type == MapOfNested;
}

inline Serializable::FieldType Serializable::fieldType(const QString &fieldName) const
{
    QVariant fieldVal = structure().value(fieldName);
    if (!fieldVal.isValid()) {
        return None;
    } else if (fieldVal.type() == QVariant::Map) {
        auto map = reinterpret_cast<const QVariantMap*>(fieldVal.constData());
        if (map->empty()) return None;
        auto &key = map->firstKey();
        auto &val = map->first();
        auto hasDeeper = val.type() == QVariant::Map;
        if (key == MAP_MARKER) {
            return hasDeeper ? MapOfNested : Map;
        } else if (key == LIST_MARKER) {
            return hasDeeper ? ContainerOfNested : Container;
        } else {
            return Nested;
        }
    } else {
        return Field;
    }
}

inline bool Serializable::wasUpdated(const QString &fieldName) const
{
    return m_deserialized.contains(qHash(fieldName));
}

inline Serializable *Serializable::parent()
{
    return m_parent;
}

inline const Serializable *Serializable::parent() const
{
    return m_parent;
}

inline void Serializable::setParent(Serializable *parent)
{
    m_parent = parent;
}

inline QMetaProperty Serializable::getProperty(const QString &propName) const
{
    auto propIndex = metaObject()->indexOfProperty(propName.toStdString().c_str());
    return metaObject()->property(propIndex);
}

inline bool Serializable::deserializeOk(const QString &fieldName) const {
    return m_deserialized.contains(qHash(fieldName)) || fieldHasDefault(fieldName);
}
inline bool Serializable::fieldHasDefault(const QString &name) const {
    auto propIndex = metaObject()->indexOfProperty((name + QStringLiteral("_hasDefault")).toStdString().c_str());
    return (propIndex > -1) ? this->readProp(metaObject()->property(propIndex)).toBool() : false;
}
inline void Serializable::markUpdated(const QString& fieldName){
    m_deserialized.insert(qHash(fieldName));
}

template<class Target>
bool Serializable::is() const
{
    return metaObject()->inherits(&Target::staticMetaObject);
}

template<class Target>
Target *Serializable::as()
{
    return is<Target>() ? static_cast<Target*>(static_cast<void*>(this)) : nullptr;
}

template<class Target>
const Target *Serializable::as() const
{
    return const_cast<Serializable*>(this)->as<Target>();
}

template<class T>
T fromQMap(const QVariantMap &src)
{
    static_assert(Priv::is_nested<T>::value, "Must Inherit from Serializable!");
    T result{};
    result.deserialize(src);
    return result;
}

template<class T>
QList<T> fromQList(const QVariantList &src)
{
    static_assert(Priv::is_nested<T>::value, "Must Inherit from Serializable!");
    QList<T> result;
    for (auto &obj : src) {
        T current{};
        current.deserialize(obj.toMap());
        result.append(current);
    }
    return result;
}

template<typename T>
QMap<QString, T> convertQMap(const QVariantMap &src)
{
    static_assert(QMetaTypeId2<T>::Defined, "Convertion to non-registered type prohibited!");
    QMap<QString, T> result;
    for (auto iter = src.constBegin(); iter != src.constEnd(); ++iter) {
        if (iter.value().canConvert<T>()) {
            result.insert(iter.key(), iter.value().value<T>());
        }
    }
    return result;
}
template<typename T>
QList<T> convertQList(const QVariantList &src)
{
    static_assert(QMetaTypeId2<T>::Defined, "Convertion to non-registered type prohibited!");
    QList<T> result;
    for (auto iter = src.constBegin(); iter != src.constEnd(); ++iter) {
        if (iter->canConvert<T>()) {
            result.append(iter->value<T>());
        }
    }
    return result;
}

}

#endif // SERIALIZER_H
