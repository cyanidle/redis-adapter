#ifndef RADAPTER_JSON_STATE_H
#define RADAPTER_JSON_STATE_H
#include "jsondict/jsondict.h"
#include "serializable/serializableobject.h"
#include "serializable/bindable.hpp"
#include "state/private/privjsonstateqobject.h"
#include "validators/validator_fetch.h"
#define STATE_PRIVATE_ATTR "state_private"
class JsonDict;
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

template <typename Target>
struct MarkPrivate : Target
{
    FIELD_SUPER(Target)
protected:
    const QStringList &attributes() const {
        static const QStringList attrs = Target::attributes() + QStringList{STATE_PRIVATE_ATTR};
        return attrs;
    }
};

struct RADAPTER_API Json : protected Serializable::Object, State::Private::JsonStateQObject {
    Q_GADGET
public:
    using Serializable::Object::field;
    using Serializable::Object::fields;
    using Serializable::Object::schema;
    using Serializable::Object::metaObject;
    QString logFields() const;
    JsonDict send() const;
    JsonDict send(const Serializable::IsFieldCheck &field) const;
    //! \warning fields like speed__vent get treated as nested (speed:vent)
    bool updateWith(const JsonDict &data);
    template <typename...Args>
    void afterUpdate(Args&&...args) {
        connect(this, &Json::wasUpdated, std::forward<Args>(args)...);
    }
private:
    bool update(const QVariantMap &data) override;
    JsonDict send(const QString &fieldName) const;
    template <typename T> friend struct Serializable::PlainField;
    template <typename T> friend struct Serializable::NestedField;
    template <typename T> friend struct Serializable::PlainSequence;
    template <typename T> friend struct Serializable::NestedSequence;
    template <typename T> friend struct Serializable::PlainMapping;
    template <typename T> friend struct Serializable::NestedMapping;
};
using Serializable::Plain;
using Serializable::Sequence;
using Serializable::Mapping;
using Serializable::Bindable;
} // namespace State
#define IS_STATE \
    IS_SERIALIZABLE_BASE \
template <typename T> using Plain = ::State::Plain<T>; \
template <typename T> using Sequence = ::State::Sequence<T>; \
template <typename T> using Mapping = ::State::Mapping<T>; \
template <typename T> using Bindable = ::State::Bindable<T>;


#endif //RADAPTER_JSON_STATE_H
