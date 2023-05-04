#ifndef VALIDATOR_FETCH_H
#define VALIDATOR_FETCH_H

#include "private/validators_pr.h"

namespace Validator {

template <typename...Args>
void registerValidator(Function<Args...> func, const QStringList &names) {
    Private::registerImpl(new Factory<Executor<Args...>, Function<Args...>>(func), names);
}

template <typename State, typename...Args>
void registerStatefulValidator(StatefulFunction<State, Args...> func, const QStringList &names) {
    Private::registerImpl(new Factory<StatefulExecutor<State, Args...>, StatefulFunction<State, Args...>>(func), names);
}

struct Fetched {
    static void initializeVariantFetching();
    Fetched() = default;
    Fetched(const QString &name);
    Fetched(const Fetched &other);
    Fetched& operator=(const Fetched &other);
    Fetched(Fetched &&other);
    Fetched &operator=(Fetched &&other);
    bool validate(QVariant &target) const;
    const QString &name() const;
    bool isValid() const;
    operator bool() const;
private:
    QSharedPointer<IExecutor> m_executor;
    QString m_name;
};

} //namespace Validator
Q_DECLARE_METATYPE(Validator::Fetched)
#endif // VALIDATOR_FETCH_H
