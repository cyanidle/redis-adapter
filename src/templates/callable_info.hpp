#ifndef CALLABLE_INFO_HPP
#define CALLABLE_INFO_HPP
#include "private/global.h"
namespace Radapter {

template<class, class = void>
struct has_call_operator
    : std::false_type
{};

template<class T>
struct has_call_operator<T, decltype(&T::operator(), void())>
    : std::true_type
{};

template<class T>
struct MethodInfo {
    enum {
        IsMethod = false,
        ArgumentCount = -1,
    };
};
template<class C, class R, class... A>
struct MethodInfo<R(C::*)(A...)> //method pointer
{
    enum {
        IsMethod = true,
        ArgumentCount = sizeof...(A),
    };
    using ClassType = C;
    using ReturnType = R;
    using ArgsTuple = std::tuple<A...>;
    using Signature = R(C::*)(A...);
    using SignatureNoClass = R(*)(A...);
};
template<class C, class R, class... A>
struct MethodInfo<R(C::*)(A...) const> : MethodInfo<R(C::*)(A...)> {}; //const method pointer
template<class C, class R, class... A>
struct MethodInfo<R(C::*)(A...) volatile> : MethodInfo<R(C::*)(A...)> {}; //volatile


template<class T>
struct FuncInfo {
    enum {
        IsFunction = false,
        ArgumentCount = -1
    };
};

template<typename R, class... A>
struct FuncInfo<R(A...)> {
    enum {
        IsFunction = true,
        ArgumentCount = sizeof...(A),
    };
    using ReturnType = R;
    using ArgsTuple = std::tuple<A...>;
    using Signature = R(A...);
};

template<class T, typename = void>
struct is_iterator : std::false_type {};

template<class T>
struct is_iterator<T, std::void_t<
                          typename T::value_type,
                          typename T::iterator_category
                          >> : std::true_type {};

template<class T>
constexpr bool is_iterator_v = is_iterator<T>::value;

template <typename T, typename = void>
struct LambdaInfo
{
    enum {
        ArgumentCount = -1,
        IsLambda = false,
    };
};

template <typename T>
struct LambdaInfo<T, typename std::enable_if<has_call_operator<T>::value>::type>
    : public LambdaInfo<decltype(&T::operator())>
{
    using typename LambdaInfo<decltype(&T::operator())>::Signature;
    using typename LambdaInfo<decltype(&T::operator())>::AsStdFunction;
    using typename LambdaInfo<decltype(&T::operator())>::ReturnType;
    enum {
        ArgumentCount = LambdaInfo<decltype(&T::operator())>::ArgumentCount,
        IsLambda = true,
    };
};

// for pointers to member function
template <typename ClassType, typename R, typename... Args>
struct LambdaInfo<R(ClassType::*)(Args...) const> {
    enum { ArgumentCount = sizeof...(Args) };
    using ReturnType = R;
    using Signature = R(ClassType::*)(Args...) const;
    typedef std::function<R (Args...)> AsStdFunction;
};

// for pointers to member function
template <typename ClassType, typename R, typename... Args>
struct LambdaInfo<R(ClassType::*)(Args...) > {
    enum { ArgumentCount = sizeof...(Args) };
    using ReturnType = R;
    using Signature = R(ClassType::*)(Args...);
    typedef std::function<R (Args...)> AsStdFunction;
};

// for function pointers
template <typename R, typename... Args>
struct LambdaInfo<R (*)(Args...)>  {
    enum { ArgumentCount = sizeof...(Args) };
    using ReturnType = R;
    using Signature = R (*)(Args...);
    typedef std::function<R (Args...)> AsStdFunction;
};

template<typename Func>
auto as_function(Func&&func) -> typename std::enable_if<FuncInfo<Func>::IsFunction, std::function<typename FuncInfo<Func>::Signature>>::type
{
    return std::function<typename FuncInfo<Func>::Signature>(std::forward<Func>(func));
}

template<typename Lambda>
auto as_function(Lambda&& lambda) -> typename std::enable_if<!FuncInfo<Lambda>::IsFunction, typename LambdaInfo<Lambda>::AsStdFunction>::type
{
    return typename LambdaInfo<Lambda>::AsStdFunction(lambda);
}
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
