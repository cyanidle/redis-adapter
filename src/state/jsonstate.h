#ifndef RADAPTER_JSON_STATE_H
#define RADAPTER_JSON_STATE_H
#include "serializable/serializableobject.h"
#include "serializable/bindable.hpp"
#include "state/private/privjsonstateqobject.h"
#include "validators/validator_fetch.h"
#define STATE_PRIVATE_ATTR "state_private"
class JsonDict;
namespace State {
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
    enum ValidatorRole {
        Update = 1 << 1,
        Send = 1 << 2,
        UpdateSend = Update | Send
    };
    Q_ENUM(ValidatorRole)
    Q_DECLARE_FLAGS(ValidateOn, ValidatorRole)
    using Serializable::Object::field;
    using Serializable::Object::fields;
    using Serializable::Object::schema;
    using Serializable::Object::metaObject;
    void addValidatorTo(const QString &field, const QString &validator, ValidateOn role = Update);
    void addValidatorTo(const Serializable::IsFieldCheck &field, const QString &validator, ValidateOn role = Update);
    void clearValidators(const QString &field, ValidateOn role);
    void clearValidators(const Serializable::IsFieldCheck &field, ValidateOn role);
    QString logFields() const;
    JsonDict send(const Serializable::IsFieldCheck &field) const;
    JsonDict send(const QString &fieldName = {}) const;
    //! \warning fields like speed__vent get treated as nested (speed:vent)
    bool updateWith(const JsonDict &data);
    template <typename...Args>
    void afterUpdate(Args&&...args) {
        connect(this, &Json::wasUpdated, std::forward<Args>(args)...);
    }
private:
    bool update(const QVariantMap &data) override;
    void fillCache() const;
    QMap<QString, QList<Validator::Fetched>> m_updateValidators;
    QMap<QString, QList<Validator::Fetched>> m_sendValidators;
    mutable QMap<QString, QStringList> m_nestedFieldsCache;
    mutable bool cacheFilled{false};
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
