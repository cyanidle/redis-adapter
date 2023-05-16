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
IExecutor *fetchImpl(const QString &name, const QVariantList &args);
template <typename...Args>
using DecTuple = std::tuple<std::decay_t<Args>...>;
template<int index, typename...Args>
void tryPopulate(DecTuple<Args...> &tup, const QVariantList &args) {
    if constexpr (index == sizeof...(Args)) {
        return;
    } else {
        using T = std::tuple_element_t<index, DecTuple<Args...>>;
        auto copy = args.at(index);
        if (!copy.convert(QMetaType::fromType<T>())) {
            throw std::runtime_error("Could not convert argument ["+std::to_string(index)+"] to type: "+QMetaType::fromType<T>().name());
        }
        std::get<index>(tup) = copy.value<T>();
        tryPopulate<index + 1, Args...>(tup, args);
    }
}
template <typename...Args>
DecTuple<Args...> unpackArgs(const QVariantList &args) {
    constexpr auto wantedCount = sizeof...(Args);
    if (wantedCount != args.size()) {
        throw std::runtime_error(std::string("Arguments list invalid! Wanted: ")
                                 + std::to_string(wantedCount)
                                 + ". Actual: " + std::to_string(args.size()));
    }
    DecTuple<Args...> result;
    tryPopulate<0, Args...>(result, args);
    return result;
}
}
class IExecutor
{
public:
    enum Flag {
        None = 0,
        IsStateful = 1 << 1
    };
    Q_DECLARE_FLAGS(Flags, Flag)
    IExecutor(Flags flags = {}) : flags(flags) {}
    constexpr bool isStateful() const {
        return flags.testFlag(IsStateful);
    }
    const Flags flags;
    virtual IExecutor *newCopy() const = 0;
    virtual bool validate(QVariant &target) = 0;
    virtual ~IExecutor() = default;
};

class IFactory
{
public:
    virtual const QString &signature() const = 0;
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
    const QString &signature() const override {
        static QString sig(typeid(Func).name());
        return sig;
    }
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
    static_assert(Radapter::CheckAll<Args...>::MetaDefined(), "Arguments must be registered with Q_DECLARE_METATYPE()");
    IExecutor *newCopy() const override {
        return new Executor(func, args);
    }
    Executor(Function<Args...> func, const Private::DecTuple<Args...> &args) :
        IExecutor(),
        func(func),
        args(args)
    {}
    Executor(Function<Args...> func, const QVariantList &args) :
        Executor(func, Private::unpackArgs<Args...>(args))
    {}
    bool validate(QVariant &target) override final {
        auto proxy = [&target, this](Args...args){
            return func(target, args...);
        };
        return std::apply(proxy, args);
    }
private:
    Function<Args...> func;
    Private::DecTuple<Args...> args;
};

template <typename State, typename...Args>
class StatefulExecutor : public IExecutor
{
public:
    static_assert(Radapter::CheckAll<Args...>::MetaDefined(), "Arguments must be registered with Q_DECLARE_METATYPE()");
    IExecutor *newCopy() const override {
        return new StatefulExecutor(func, args);
    }
    StatefulExecutor(StatefulFunction<State, Args...> func, const Private::DecTuple<Args...> &args) :
        IExecutor(IExecutor::IsStateful),
        func(func),
        args(args),
        state{}
    {}
    StatefulExecutor(StatefulFunction<State, Args...> func, const QVariantList &args) :
        StatefulExecutor(func, Private::unpackArgs<Args...>(args))
    {}
    bool validate(QVariant &target) override final {
        auto proxy = [&target, this](Args...args){
            return func(target, state, std::forward<Args>(args)...);
        };
        return std::apply(proxy, args);
    }
private:
    StatefulFunction<State, Args...> func;
    Private::DecTuple<Args...> args;
    State state;
};
}
#endif
