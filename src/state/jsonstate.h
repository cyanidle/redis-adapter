#ifndef RADAPTER_JSON_STATE_H
#define RADAPTER_JSON_STATE_H
#include "broker/workers/worker.h"
#include "jsondict/jsondict.h"
#include "serializable/serializableobject.h"
#include "serializable/bindable_field.hpp"
#include "state/private/privjsonstateqobject.h"
#include "validators/validator_fetch.h"
#define STATE_PRIVATE_ATTR "state_private"
class JsonDict;
namespace Settings{struct Serializable;}
namespace State {

template <typename T, typename Func>
void callIfValid(const JsonDict &data, const QString &path, Func func)
{
    auto copy = data.value(path);
    if (!copy.convert(QMetaType::fromType<T>())) return;
    func(copy.value<T>());
}

template <typename T, typename User, typename Method>
void callIfValid(const JsonDict &data, const QString &path, User *user, Method method)
{
    auto copy = data.value(path);
    if (!copy.convert(QMetaType::fromType<T>())) return;
    (user->*method)(copy.value<T>());
}

template <typename T, typename User, typename Method>
void callIfValid(const JsonDict &data, const QString &path, const User *user, Method method)
{
    auto copy = data.value(path);
    if (!copy.convert(QMetaType::fromType<T>())) return;
    (user->*method)(copy.value<T>());
}

struct RADAPTER_API Json : protected Serializable::Object {
    Q_GADGET
    struct Private;
public:
    Q_DISABLE_COPY_MOVE(Json)
    Json(Radapter::Worker *parent = nullptr);
    ~Json();
    using Serializable::Object::field;
    using Serializable::Object::fields;
    using Serializable::Object::schema;
    using Serializable::Object::structure;
    using Serializable::Object::metaObject;
    QString logInfo() const;
    JsonDict send() const;
    JsonDict send(const Serializable::IsFieldCheck &field) const;
    bool updateWith(const Settings::Serializable &setting);
    bool updateWith(const JsonDict &data);
    //! Will call a callback after update with
    /// possible args (State::Json *state)
    template <typename...Args>
    void afterUpdate(Args&&...args) {
        obj->connect(d, &State::Private::JsonStateQObject::wasUpdated, std::forward<Args>(args)...);
    }
    //! Will call a callback before update with
    /// possible args (const JsonDict &data, State::Json *state)
    template <typename...Args>
    void beforeUpdate(Args&&...args) {
        obj->connect(d, &State::Private::JsonStateQObject::beforeUpdate, std::forward<Args>(args)...);
    }
private:
    State::Private::JsonStateQObject *obj;
    Private *d;

    bool update(const QVariantMap &data) override;
    JsonDict send(const QString &fieldName) const;
    template <typename T> friend struct Serializable::PlainField;
    template <typename T> friend struct Serializable::NestedField;
    template <typename T> friend struct Serializable::PlainSequence;
    template <typename T> friend struct Serializable::NestedSequence;
    template <typename T> friend struct Serializable::PlainMapping;
    template <typename T> friend struct Serializable::NestedMapping;
};
} // namespace State
#define IS_STATE \
    _BASE_IS_SERIALIZABLE
template <typename T> using Plain = ::Serializable::Plain<T>; \
template <typename T> using Sequence = ::Serializable::Sequence<T>; \
template <typename T> using Mapping = ::Serializable::Mapping<T>; \
template <typename T> using Bindable = ::Serializable::Bindable<T>;


#endif //RADAPTER_JSON_STATE_H
