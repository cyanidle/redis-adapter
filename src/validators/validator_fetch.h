#ifndef VALIDATOR_FETCH_H
#define VALIDATOR_FETCH_H

#include "private/validators_pr.h"

namespace Validator {
using Function = bool (*)(QVariant &target, const QVariantList &args, QVariant& state);
Function fetchFunction(const char *name);
Function fetchFunction(const QLatin1String &name);
Function fetchFunction(const QString &name);
const QStringList available();
template <typename T>
void makeFetchable() {
    Private::add(T::validate, T::_aliases(), T::_aliases_count());
}
template <typename T, typename...Names>
void makeFetchable(Names...aliases) {
    Private::add(T::validate, aliases...);
}
void makeFetchable(Function validatingFunction, const QStringList &aliases);
struct Fetched {
    Fetched();
    Fetched(const QString &name);
    static void addArgsFor(const QString &name, const QVariantList &args, const QString &newName);
    static void initialize();
    bool validate(QVariant &target) const;
    bool validate(QVariant &target, QVariant& state) const;
    const QString &name() const;
    bool isValid() const;
    operator bool() const;
    bool operator<(const Fetched &other) const;
    bool operator==(const QVariant &variant) const;
    bool operator!=(const QVariant &variant) const;
private:
    QString m_name;
    QVariantList m_args;
    ::Validator::Function m_executor;
};
} //namespace Validator
Q_DECLARE_METATYPE(Validator::Fetched)
#endif // VALIDATOR_FETCH_H
