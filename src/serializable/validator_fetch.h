#ifndef VALIDATOR_FETCH_H
#define VALIDATOR_FETCH_H

#include <QMetaType>
#include "common_fields.hpp"

namespace Validator {
using Function = bool (*)(QVariant&);
struct Executor {
    Executor(Function func);
    bool validate(QVariant &target) const;
private:
    Function m_func;
};
const Executor *fetch(const char *name);
const Executor *fetch(const QLatin1String &name);
const Executor *fetch(const QString &name);
QString nameOf(const Executor *validator);


namespace Private {
int add(Function func, const char **names, int count);
int add(Function func, const char *name);
template <typename...Names>
int add(Function func, const char *name, const char *name2, Names...names) {
    return add(func, name) + add(func, name2, names...);
}
} //namespace Private
template <typename T>
void makeFetchable() {
    Private::add(T::validate, T::_names(), T::_names_count());
}
template <typename T, typename...Names>
void makeFetchable(Names...names) {
    Private::add(T::validate, names...);
}
} //namespace Validator
Q_DECLARE_METATYPE(const Validator::Executor*)
#define VALIDATOR_DECLARE_NAMES(...) \
static const char **_names() { \
static const char *_names_impl[]{__VA_ARGS__};\
return _names_impl;\
} \
static int _names_count() { \
const char *_names_impl[]{__VA_ARGS__};\
return sizeof(_names_impl)/sizeof(const char *); \
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
