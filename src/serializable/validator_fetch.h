#ifndef VALIDATOR_FETCH_H
#define VALIDATOR_FETCH_H

#include <QMetaType>
#include "common_fields.hpp"

namespace Validator {
using Function = bool (*)(QVariant&);
struct Executor {
    Executor(Function func);
    bool validate(QVariant &target) const;
    QString name() const;
    QStringList aliases() const;
private:
    Function m_func;
};
const Executor *fetch(const char *name);
const Executor *fetch(const QLatin1String &name);
const Executor *fetch(const QString &name);
QString nameOf(const Executor *validator);


namespace Private {
int add(Function func, const char **aliases, int count);
int add(Function func, const char *alias);
template <typename...Names>
int add(Function func, const char *alias, const char *alias2, Names...aliases) {
    return add(func, alias) + add(func, alias2, aliases...);
}
} //namespace Private
template <typename T>
void makeFetchable() {
    Private::add(T::validate, T::_aliases(), T::_aliases_count());
}
template <typename T, typename...Names>
void makeFetchable(Names...aliases) {
    Private::add(T::validate, aliases...);
}
} //namespace Validator
Q_DECLARE_METATYPE(const Validator::Executor*)
#define VALIDATOR_DECLARE_ALIASES(...) \
static const char **_aliases() { \
static const char *_aliases_impl[]{__VA_ARGS__};\
return _aliases_impl;\
} \
static int _aliases_count() { \
const char *_aliases_impl[]{__VA_ARGS__};\
return sizeof(_aliases_impl)/sizeof(const char *); \
}


namespace Serializable {
struct Validator : public PlainField<const ::Validator::Executor*> {
    using typename PlainField<const ::Validator::Executor*>::valueType;
    using typename PlainField<const ::Validator::Executor*>::valueRef;
    using PlainField<const ::Validator::Executor*>::PlainField;
    using PlainField<const ::Validator::Executor*>::operator=;
    using PlainField<const ::Validator::Executor*>::operator==;
    bool updateWithVariant(const QVariant &source);
    QVariant readVariant() const;
};

} //namespace Serializable

#endif // VALIDATOR_FETCH_H
