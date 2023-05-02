#ifndef RADAPTER_JSON_STATE_H
#define RADAPTER_JSON_STATE_H
#include "serializable/serializableobject.h"
#include "serializable/bindable.hpp"
#include "state/private/privjsonstateqobject.h"
#include "validators/validator_fetch.h"
class JsonDict;
namespace State {
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
    void fillCache() const;
    QMap<QString, Validator::Fetched> m_updateValidators;
    QMap<QString, Validator::Fetched> m_sendValidators;
    mutable QMap<QString, QStringList> m_nestedFieldsCache;
    mutable bool cacheFilled{false};
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
