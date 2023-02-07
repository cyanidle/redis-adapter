#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <QVariant>
#include <QMetaProperty>
#include <QMetaObject>
#include <type_traits>
#include <stdexcept>
#include "templates/metaprogramming.hpp"
#include "private/global.h"
#include "private/brokerlogging.h"
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
    bool deserialize(const QVariantMap &src, bool pedantic = true, const QString &prefix = "~");
    QVariantMap serialize() const;
    QVariantMap serializeField(const QString &fieldName) const;
    virtual const QStringList& fields() const = 0;
    virtual const QVariantMap& structure() const = 0;
    QVariant value(const QString &fieldName) const;
    FieldType fieldType(const QString &fieldName) const;
    bool wasUpdated() const;
protected:
    const QList<quint32> &updatedFieldsHash() const;
    void updateWasInit(const QString &name, bool remove = false);
    const QString &prefix() const;
    bool wasFullyDeserialized() const;
    bool isPedantic() const;
    bool fieldHasDefault(const QString &name) const;
    QVariant readMock() const;
    bool initMock(const QVariant &src) noexcept;
    void noAction() const noexcept {}
    QVariantMap structureUncached() const;
    bool isPropertyFromMacro(const std::string &name) const;
    virtual ~Serializable() = default;
private:
    virtual QVariant readProp(const QMetaProperty &prop) const = 0;
    virtual bool writeProp(const QMetaProperty &prop, const QVariant &val) = 0;
    bool m_pedantic{true};
    QString m_prefix{"~"};
    QList<quint32> m_wasInit{};
    virtual void _settings_impl_OnInit(){};
    virtual void _settings_impl_PostInit(){};
    virtual void _settings_impl_OnFail(){};
    virtual void _settings_impl_OnSuccess(){};
    virtual const QMetaObject * metaObject() const = 0;
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

template<class T>   T fromQMap(const QVariantMap &src, bool pedantic = true);
template<class T>   QList<T> fromQList(const QVariantList &src, bool pedantic = true);
template<class T>   QMap<QString, T> convertQMap(const QVariantMap &src);
template<class T>   QList<T> convertQList(const QVariantList &src);
template<class T>   using QStringMap = QMap<QString, T>;

}

#define COMMA ,
#define DEFAULT
#define ROOT_PREFIX QStringLiteral("~")
#define NO_INIT initMock
#define NO_READ readMock
#define NO_ACTION noAction
#define MAP_MARKER QStringLiteral("<map>")
#define LIST_MARKER QStringLiteral("<list>")
#define _NO_ARGS
#define _FIND_CLASS std::decay<decltype(*this)>::type
#define _NAME_OF_TYPE(type) typeid(type).name()
#define _VAL_NAME(name) QStringLiteral(#name)
#define _BASE_MACRO(type, name, decay, concept, concept_args, props, funcs, oninit, set_args, read_args, map_args, vargs_str, ...) \
public:\
    type name = {__VA_ARGS__}; \
    bool \
    name##_WasUpdated() \
    const {return wasUpdated() && updatedFieldsHash().contains(qHash(QStringLiteral(#name)));} \
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
    Q_PROPERTY(QVariant name##_hasDefault READ name##_hasDefault) \
    Q_PROPERTY(QVariantMap name##_getMap READ name##_getMap) \
    QVariantMap name##_getMap() const {_check_if_has_is_serializable(); \
        return concept<type, decay, concept_args>:: \
        get_map(this, name, _VAL_NAME(name), map_args);}\
    void write_##name(const QVariant &v) { \
    if (concept<type, decay, concept_args>:: \
        set_val(this, name, v, _VAL_NAME(name), set_args)) {oninit};} \
    QVariant read_##name() const { \
        return concept<type, decay, concept_args>:: \
        get_val(this, name, _VAL_NAME(name), read_args);} \
public:

#define SERIAL_CUSTOM(type, name, setter, getter, ...) \
public: \
    type name = {__VA_ARGS__}; \
    protected: \
    static type name##_Default() {return type{__VA_ARGS__};} \
    constexpr static bool name##_hasDefault() {return !QLatin1String(#__VA_ARGS__).isEmpty();} \
    private: \
    Q_PROPERTY(QVariant name READ _read_##name WRITE write_##name) \
    Q_PROPERTY(QVariant name##_hasDefault READ name##_hasDefault) \
    Q_PROPERTY(QVariantMap name##_getMap READ name##_getMap) \
    QVariantMap name##_getMap () const { \
        _check_if_has_is_serializable(); \
        return {{#name, _NAME_OF_TYPE(type)}}; \
} \
    QVariant _read_##name () const { \
        return QVariant(getter()); \
} \
    void write_##name (const QVariant & newVal) { \
        if (setter(newVal)) { \
            updateWasInit(#name); \
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
    static QVariant map(const Decay& val, int typeId, is_nested_t<Decay, int>* = 0) {
        Q_UNUSED(typeId)
        return val.structure();
    }
    template<typename Wrapped, typename Decay>
    static QVariant map(const Decay& val, int typeId, not_nested_t<Decay, int>* = 0) {
        Q_UNUSED(val)
        return QVariant::fromValue(typeId);
    }
    template<typename Wrapped, typename Decay>
    static QVariant map(const Decay& val, int typeId, is_enum_t<Decay, int>* = 0) {
        Q_UNUSED(val)
        return QVariant::fromValue(typeId);
    }
    template<typename Wrapped, typename Decay>
    static QVariant map(const Wrapped& val, int typeId, is_shared_ptr_t<Wrapped, int>* = 0) {
        Q_UNUSED(val)
        return QVariant::fromValue(typeId);
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
        if (val) return val->serialize();
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
    template<typename Holder, typename Wrapped, typename Decay, typename Map>
    static bool /* Mapped extraction */
    set(Holder *obj, Decay& val, const QVariant &newVal, const Map &mapper, const QString &name = {}) {
        Radapter::Unused(obj, name);
        static_assert(Radapter::ContainerInfo<Map>::has_contains_key, "Map must provide .key(value) .value(key) .contains(value) methods!");
        auto keyVal = newVal.value<Radapter::container_key_t<Map>>();
        if (mapper.contains(keyVal)) {
            val = mapper.value(keyVal);
            return true;
        }
        return false;
    }
    template<typename Holder, typename Wrapped, typename Decay>
    static bool /* Nested extraction */
    set(Holder *obj, Decay& val, const QVariant &newVal, const QString &name = {}, is_nested_t<Decay, int>* = 0) {
        return val.deserialize(newVal.toMap(), obj->isPedantic(), obj->prefix() + "/" + name);
    }
    template<typename Holder, typename Wrapped, typename Decay>
    static bool /* Simple extraction */
    set(Holder *obj, Decay& val, const QVariant &newVal, const QString &name = {}, not_nested_t<Decay, int>* = 0) {
        Radapter::Unused(obj, name);
        static_assert(QMetaTypeId2<Decay>::Defined,
                "Underlying type must be registered with Q_DECLARE_METATYPE!");
        if (std::is_same<Decay, QVariant>() || newVal.canConvert<Decay>()) {
            auto actualValue = newVal.value<Decay>();
            if (actualValue != newVal) {
                settingsParsingWarn().noquote() << "Types mismatch, while deserializing -->\n Wanted: " <<
                                                   QMetaType::fromType<Decay>().name() <<
                                                   "\n Actual: " << newVal;
                return false;
            }
            val = std::move(actualValue);
            return true;
        }
        return false;
    }
    template<typename Holder, typename Wrapped, typename Decay>
    static bool /* Simple extraction */
    set(Holder *obj, Decay& val, const QVariant &newVal, const QString &name = {}, is_enum_t<Decay, int>* = 0) {
        Radapter::Unused(obj, name);
        static_assert(QMetaTypeId2<Decay>::Defined,
                "Underlying type must be registered with Q_DECLARE_METATYPE!");
        if (std::is_same<Decay, QVariant>() || newVal.canConvert<Decay>()) {
            val = newVal.value<Decay>();
            return true;
        }
        return false;
    }
    template<typename Holder, typename Wrapped, typename Decay>
    static bool /* Pointer extraction */
    set(Holder *obj, Wrapped& val, const QVariant &newVal, const QString &name = {}, is_shared_ptr_t<Wrapped, int>* = 0) {
        Radapter::Unused(obj, name);
        if (!val) val.reset(new Decay{});
        return set<Holder, Wrapped, Decay>(obj, *val, newVal, name);
    }
};

//! Concepts implement basic logic of iterating/parsing/calling of Implementations overloads (which handle different underlying types)
/// Must implement set_val, get_val, get_map, can be overloaded based on additional args (set_args, get_args, map_args)
template<typename T, typename Decay, typename... Extra>
struct FieldConcept {
    static_assert(QMetaTypeId2<Decay>::Defined,
            "Underlying type must be registered with Q_DECLARE_METATYPE!");
    template<typename Holder>
    static QVariantMap get_map(const Holder * obj, const T& val, QString &&name, int) {
        Radapter::Unused(obj, val);
        return {{name, Implementations::map<T, Decay>(val, QMetaType::fromType<Decay>().id())}};
    };
    template<typename Holder>
    static QVariant get_val(const Holder * obj, const T& val, QString &&name, int) {
        Radapter::Unused(obj, name);
        return Implementations::val<T, Decay>(val);
    }
    template<typename Holder>
    static bool set_val(Holder * obj, T& val, const QVariant& newVal, QString &&name, int) {
        if (Implementations::set<Holder, T, Decay>(obj, val, newVal, name)) {
            obj->updateWasInit(name);
            return true;
        }
        return false;
    }
    //! For automatic conversions from source QVariant to Some Other Type (Like from string in toml to expected Enum)
    /// Used in SERIAL_FIELD_MAPPED
    template<typename Holder, typename Mapping>
    static typename std::enable_if<Radapter::ContainerInfo<Mapping>::has_key &&
                                   Radapter::ContainerInfo<Mapping>::has_value &&
                                   !std::is_same<Mapping, int>::value,
                                   bool>::type
    set_val(Holder * obj, T& val, const QVariant& newVal, QString &&name, const Mapping &mapper) {
        if (Implementations::set<Holder, T, Decay, Mapping>(obj, val, newVal, mapper, name)) {
            obj->updateWasInit(name);
            return true;
        }
        return false;
    }
    template<typename Holder, typename Mapping>
    static typename std::enable_if<Radapter::ContainerInfo<Mapping>::has_key &&
                                   Radapter::ContainerInfo<Mapping>::has_value &&
                                   !std::is_same<Mapping, int>::value,
                                   QVariant>::type
    get_val(const Holder * obj, const T& val, QString &&name, const Mapping &mapper) {
        Radapter::Unused(obj, name);
        return Implementations::val<T, Decay, Mapping>(val, mapper);
    }
};

template<typename T, typename Decay, typename... Extra>
struct ContainerConcept {
    static_assert(QMetaTypeId2<Decay>::Defined,
            "Underlying type must be registered with Q_DECLARE_METATYPE!");
    template<typename Holder>
    static QVariantMap get_map(const Holder * obj, const T& val, QString &&name, int) {
        Radapter::Unused(obj, val);
        if (val.size()) {
            return {{name,
                     QVariantMap{{LIST_MARKER,
                                        Implementations::map<cont_value_t<T>, Decay>(*val.cbegin(),
                                                                               QMetaType::fromType<Decay>().id())}}}};
        }
        else return {{name,
                     QVariantMap{{LIST_MARKER,
                                        Implementations::map<cont_value_t<T>, Decay>(Decay{},
                                                                               QMetaType::fromType<Decay>().id())}}}};
    };
    template<typename Holder>
    static QVariant get_val(const Holder * obj, const T& val, QString &&name, int) {
        Radapter::Unused(obj, name);
        QVariantList result;
        for (auto &item : val) {
            result.append(Implementations::val<cont_value_t<T>, Decay>(item));
        }
        return result;
    }

    template <typename Holder, typename ValueT>
    static typename std::enable_if<Radapter::ContainerInfo<Holder>::has_push_back>::type
    append(Holder &holder, const ValueT &val) {
        holder.push_back(val);
    }
    template <typename Holder, typename ValueT>
    static typename std::enable_if<Radapter::ContainerInfo<Holder>::has_insert_one_arg>::type
    append(Holder &holder, const ValueT &val) {
        holder.insert(val);
    }
    template<typename Holder>
    static bool set_val(Holder * obj, T& val, const QVariant& newVal, QString &&name, int) {
        if (!newVal.canConvert<QVariantList>()) return false;
        auto source = newVal.toList();
        val.clear();
        val.reserve(source.size());
        bool ok = true;
        int count = 0;
        for (auto &nested : source) {
            cont_value_t<T> current{};
            if (!Implementations::set<Holder, cont_value_t<T>, Decay>(obj, current, nested, name + ":" + QString::number(count++))) {
                ok = false;
                continue;
            }
            append(val, current);
        }
        if (ok) {
            obj->updateWasInit(name);
        }
        return ok;
    }
};

template<typename T, typename Decay, typename... Extra>
struct MapConcept  {
    static_assert(QMetaTypeId2<Decay>::Defined,
            "Underlying type must be registered with Q_DECLARE_METATYPE!");
    template<typename Holder>
    static QVariantMap get_map(const Holder * obj, const T& val, QString &&name, int) {
        Radapter::Unused(obj, val);
        if (val.size()) {
            return {{name, QVariantMap{{MAP_MARKER,
                                        Implementations::map<Decay>(*val.cbegin(),
                                                                    QMetaType::fromType<Decay>().id())}}}};
        }
        else return {{name, QVariantMap{{MAP_MARKER,
                                        Implementations::map<Decay>(Decay{},
                                                                    QMetaType::fromType<Decay>().id())}}}};
    };
    template<typename Holder>
    static QVariant get_val(const Holder * obj, const T& val, QString &&name, int) {
        Radapter::Unused(obj, name);
        QVariantMap result;
        for (auto iter = val.constBegin(); iter != val.constEnd(); ++iter) {
            result[iter.key()] = std::move(Implementations::val<cont_value_t<T>, Decay>(iter.value()));
        }
        return result;
    }
    template<typename Holder>
    static bool set_val(Holder * obj, T& val, const QVariant& newVal, QString &&name, int) {
        if (!newVal.canConvert<QVariantMap>()) return false;
        val.clear();
        bool ok = true;
        auto currentMap = newVal.toMap();
        for (auto iter = currentMap.constBegin(); iter != currentMap.constEnd(); ++iter) {
            if (!Implementations::set<Holder, cont_value_t<T>, Decay>(obj, val[iter.key()], iter.value(), iter.key() + ":" + name)) {
                ok = false;
            }
        }
        if (ok) {
            obj->updateWasInit(name);
        }
        return ok;
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

#define _IMPL_H_SERIAL_POST_INIT_(func, pre, post, ...) \
private: \
    pre void _settings_impl_PostInit() override post { \
        func(__VA_ARGS__); \
} \
    public:
#define SERIAL_POST_INIT(func, ...) _IMPL_H_SERIAL_POST_INIT_(func,  ,  , __VA_ARGS__)
#define SERIAL_POST_INIT_FINAL(func, ...) _IMPL_H_SERIAL_POST_INIT_(func,  , final, __VA_ARGS__)

#define _IMPL_H_SERIAL_ON_FAIL_(func, pre, post, ...) \
    private: \
    pre void _settings_impl_OnFail() override post { \
        func(__VA_ARGS__); \
} \
    public:
#define SERIAL_ON_FAIL(func, ...) _IMPL_H_SERIAL_ON_FAIL_(func,  ,  , __VA_ARGS__)
#define SERIAL_ON_FAIL_FINAL(func, ...) _IMPL_H_SERIAL_ON_FAIL_(func,  , final, __VA_ARGS__)

#define _IMPL_H_SERIAL_PRE_INIT_(func, pre, post, ...) \
    private: \
    pre void _settings_impl_OnInit() override post { \
        func(__VA_ARGS__); \
} \
    public:
#define SERIAL_PRE_INIT(func, ...) _IMPL_H_SERIAL_PRE_INIT_(func,  ,  , __VA_ARGS__)
#define SERIAL_PRE_INIT_FINAL(func, ...) _IMPL_H_SERIAL_PRE_INIT_(func,  , final, __VA_ARGS__)

#define _IMPL_H_SERIAL_ON_SUCCESS_(func, pre, post, ...) \
    private: \
    pre void _settings_impl_OnSuccess() override post { \
        func(__VA_ARGS__); \
} \
    public:
#define SERIAL_ON_SUCCESS(func, ...) _IMPL_H_SERIAL_ON_SUCCESS_(func,  ,  , __VA_ARGS__)
#define SERIAL_ON_SUCCESS_FINAL(func, ...) _IMPL_H_SERIAL_ON_SUCCESS_(func,  , final, __VA_ARGS__)

#define IS_SERIALIZABLE \
private: \
    void _check_if_has_is_serializable() const noexcept { \
        using type = typename _FIND_CLASS; \
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
        using type = typename _FIND_CLASS; \
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
    inline bool Serializable::deserialize(const QVariantMap &src, bool pedantic, const QString &prefix) {
        m_wasInit.clear();
        m_prefix = prefix;
        m_pedantic = pedantic;
        _settings_impl_OnInit();
        int propCount = metaObject()->propertyCount();
        for(int i = 0; i < propCount; i++) {
            auto prop = metaObject()->property(i);
            auto propName = prop.name();
            if (src.contains(propName)) {
                if (isPropertyFromMacro(propName)) {
                    this->writeProp(prop, src[propName]);
                }
            }
        }
        _settings_impl_PostInit();
        if (wasFullyDeserialized()) {
            _settings_impl_OnSuccess();
            return true;
        } else {
            _settings_impl_OnFail();
            return false;
        }
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
        auto result = QVariantMap();
        auto index = metaObject()->indexOfProperty(fieldName.toStdString().c_str());
        if (index == -1) throw std::invalid_argument("Field: (" + fieldName.toStdString() + ") --> is not marked SERIAL_<MACRO>!");
        result.insert(fieldName, this->readProp(metaObject()->property(index)));
        return result;
    }

    inline QVariantMap Serializable::structureUncached() const {
        QVariantMap result;
        int propCount = metaObject()->propertyCount();
        for(int i = 0; i < propCount; i++) {
            auto propName = std::string(metaObject()->property(i).name());
            if (isPropertyFromMacro(propName)) {
#if !_SERIALIZER_PORTABLE
                auto prop = metaObject()->property(metaObject()->indexOfProperty((propName + "_getMap").c_str()));
                result.insert(this->readProp(prop).toMap());
#else
                auto subresult = this->readProp(prop).toMap();
                for (auto iter = subresult.constBegin(); iter != subresult.constEnd(); ++iter) {
                    result.insert(iter.key(), iter.value());
                }
#endif
            }
        }
        return result;
    }

    inline bool Serializable::isPropertyFromMacro(const std::string &name) const
    {
        auto defaultName = name + "_hasDefault";
        return metaObject()->indexOfProperty(defaultName.c_str()) != -1;
    }
    inline QVariant Serializable::value(const QString &fieldName) const
    {
        if (!fields().contains(fieldName)) return {};
        auto propIndex = metaObject()->indexOfProperty(fieldName.toStdString().c_str());
        auto prop = metaObject()->property(propIndex);
        return this->readProp(prop);
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

    inline bool Serializable::wasUpdated() const {return !updatedFieldsHash().isEmpty();}
    inline const QList<quint32> &Serializable::updatedFieldsHash() const {return m_wasInit;}
    inline const QString &Serializable::prefix() const {return m_prefix;}
    inline bool Serializable::wasFullyDeserialized() const {
        bool ok = true;
        for (auto &currentName : fields()) {
            if (!m_wasInit.contains(qHash(currentName))) {
                if (fieldHasDefault(currentName)) {
                    continue;
                }
                if (isPedantic()) {
                    throw std::runtime_error((std::string(metaObject()->className()) +
                                              " --> Deserialization Error of Field: (" +
                                              (prefix() + "/" + currentName).toStdString() + ")"));
                }
                ok = false;
            }
        }
        return ok;
    }
    inline bool Serializable::isPedantic() const {return m_pedantic;}
    inline bool Serializable::fieldHasDefault(const QString &name) const {
        auto propIndex = metaObject()->indexOfProperty((name + QStringLiteral("_hasDefault")).toStdString().c_str());
        return (propIndex > -1) ? this->readProp(metaObject()->property(propIndex)).toBool() : false;
    }
    inline void Serializable::updateWasInit(const QString& name, bool remove){
        if (!remove && !m_wasInit.contains(qHash(name))) {
            m_wasInit.append(qHash(name));
        } else if (remove) {
            m_wasInit.removeOne(qHash(name));
        }
    }
    inline QVariant Serializable::readMock() const {return QVariant();}
    inline bool Serializable::initMock(const QVariant &src) noexcept {Q_UNUSED(src); return true;}


    template<class T>
    T fromQMap(const QVariantMap &src, bool pedantic)
    {
        static_assert(Priv::is_nested<T>::value, "Must Inherit from Serializable!");
        T result{};
        result.deserialize(src, pedantic);
        return result;
    }

    template<class T>
    QList<T> fromQList(const QVariantList &src, bool pedantic)
    {
        static_assert(Priv::is_nested<T>::value, "Must Inherit from Serializable!");
        QList<T> result;
        for (auto &obj : src) {
            T current{};
            current.deserialize(obj.toMap(), pedantic);
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
