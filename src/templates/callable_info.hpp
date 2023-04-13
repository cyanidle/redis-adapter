#ifndef CALLABLE_INFO_HPP
#define CALLABLE_INFO_HPP
#include "metaprogramming.hpp"
namespace Radapter {

template <typename T, typename = void>
struct CallableInfo {
    enum {
        ArgumentCount = -1,
        IsCallable = false,
        IsMethod = false,
        IsFunction = false,
        IsFunctor = false,
        IsLambda = IsFunctor
    };
};

template <typename T>
struct CallableInfo<T, typename std::enable_if<has_call_operator<T>::value>::type> {
    using Info = LambdaInfo<T>;
    enum {
        ArgumentCount = Info::ArgumentCount,
        IsCallable = true,
        IsMethod = false,
        IsFunction = false,
        IsFunctor = true,
        IsLambda = IsFunctor
    };
};

template <typename T>
struct CallableInfo<T, typename std::enable_if<MethodInfo<T>::IsMethod>::type> {
    using Info = MethodInfo<T>;
    enum {
        ArgumentCount = Info::ArgumentCount,
        IsCallable = true,
        IsMethod = true,
        IsFunction = false,
        IsFunctor = false,
        IsLambda = IsFunctor
    };
};

template <typename T>
struct CallableInfo<T, typename std::enable_if<FuncInfo<T>::IsFunction>::type> {
    using Info = FuncInfo<T>;
    enum {
        ArgumentCount = Info::ArgumentCount,
        IsCallable = true,
        IsMethod = false,
        IsFunction = true,
        IsFunctor = false,
        IsLambda = IsFunctor
    };
};

}

#endif // CALLABLE_INFO_HPP
