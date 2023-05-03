#ifndef VALIDATORS_PRIVATE_H
#define VALIDATORS_PRIVATE_H

#include "private/global.h"
#include "templates/metaprogramming.hpp"

namespace Validator {
class IFactory;
class IExecutor;
namespace Private {
void registerImpl(IFactory *factory, const QStringList &names);
IExecutor *fetchImpl(const QString &name);
template<int index, typename...Args>
void tryPopulate(std::tuple<Args...> &tup, const QVariantList &args) {
    if constexpr (index == sizeof...(Args)) {
        return;
    } else {
        using T = std::decay_t<std::tuple_element_t<index, std::tuple<Args...>>>;
        auto copy = args.at(index);
        if (!copy.convert(QMetaType::fromType<T>())) {
            throw std::runtime_error("Could not convert argument ["+std::to_string(index)+"] to type: "+typeid(T).name());
        }
        std::get<index>(tup) = copy.value<T>();
        tryPopulate<index + 1, Args...>(tup, args);
    }
}
template <typename...Args>
std::tuple<Args...> unpackArgs(const QVariantList &args) {
    constexpr auto wantedCount = sizeof...(Args);
    if (wantedCount != args.size()) {
        throw std::runtime_error(std::string("Arguments list invalid! Wanted: ")
                                 + std::to_string(wantedCount)
                                 + ". Actual: " + std::to_string(args.size()));
    }
    std::tuple<Args...> result;
    tryPopulate<0, Args...>(result, args);
    return result;
}
template <typename...Args>
struct CheckArgs {
    enum {
        Empty = !sizeof...(Args),
        Copyable = Empty || Radapter::all_true<std::is_copy_constructible<Args>::value...>(),
        MetaDefined = Empty || Radapter::all_true<QMetaTypeId2<Args>::Defined...>(),
    };
};
}
class IExecutor
{
public:
    virtual IExecutor *newCopy() const = 0;
    virtual bool validate(QVariant &target) = 0;
    virtual ~IExecutor() = default;
};

class IFactory
{
public:
    virtual IExecutor* create(const QVariantList &args) = 0;
    virtual ~IFactory() = default;
};


template <typename...Args>
using Function = bool(*)(QVariant&, Args...);

template <typename State, typename...Args>
using StatefulFunction = bool(*)(QVariant&, State&, Args...);

template <typename Exec, typename Func>
class Factory : public IFactory
{
public:
    Factory(Func func) : func(func)
    {}
    IExecutor* create(const QVariantList &args) override
    {
        return new Exec(func, args);
    }
private:
    Func func;
};

template <typename...Args>
class Executor : public IExecutor
{
public:
    static_assert(Private::CheckArgs<Args...>::MetaDefined, "Arguments must be registered with Q_DECLARE_METATYPE()");
    IExecutor *newCopy() const override {
        return new Executor(func, raw);
    }
    Executor(Function<Args...> func, const QVariantList &args) :
        func(func),
        args(Private::unpackArgs<Args...>(args)),
        raw(args)
    {}
    bool validate(QVariant &target) override final {
        auto proxy = [&target, this](Args...args){
            return func(target, args...);
        };
        return std::apply(proxy, args);
    }
private:
    Function<Args...> func;
    std::tuple<std::decay_t<Args>...> args;
    QVariantList raw;
};

template <typename State, typename...Args>
class StatefulExecutor : public IExecutor
{
public:
    static_assert(Private::CheckArgs<Args...>::MetaDefined, "Arguments must be registered with Q_DECLARE_METATYPE()");
    IExecutor *newCopy() const override {
        return new StatefulExecutor(func, raw);
    }
    StatefulExecutor(StatefulFunction<State, Args...> func, const QVariantList &args) :
        func(func),
        args(Private::unpackArgs<Args...>(args)),
        state{},
        raw(args)
    {}
    bool validate(QVariant &target) override final {
        auto proxy = [&target, this](Args...args){
            return func(target, state, std::forward<Args>(args)...);
        };
        return std::apply(proxy, args);
    }
private:
    StatefulFunction<State, Args...> func;
    std::tuple<std::decay_t<Args>...> args;
    State state;
    QVariantList raw;
};
}
#endif
