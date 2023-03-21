#ifndef METAPROGRAMMING_HPP
#define METAPROGRAMMING_HPP

#include <functional>
#include <type_traits>
#include <tuple>
#include <QMetaObject>

namespace Radapter {

template<typename T> struct has_QGadget_Macro {
    template<typename> static std::false_type impl(...);
    template<typename U> static auto impl(int) ->
        decltype(std::declval<U>().qt_check_for_QGADGET_macro(), std::true_type());
    enum { Value = decltype(impl<T>(0))::value};
};

template<typename T> struct has_QObject_Macro {
    template<typename> static std::false_type impl(...);
    template<typename U> static auto impl(int) ->
        decltype(std::declval<U>().qt_metacall(QMetaObject::Call::IndexOfMethod, 1, nullptr), std::true_type());
    enum { Value = decltype(impl<T>(0))::value};
};


template<typename T> struct has_MetaObject_Func {
    template <typename U>
    static char test(const QMetaObject * (U::*)() const);
    static int test(const QMetaObject * (T::*)() const);
    enum { Value =  sizeof(test(&T::metaObject)) == sizeof(int) };
};

template<typename T>
struct Gadget_With_MetaObj {
    enum { Value = has_MetaObject_Func<T>::Value &&
                   has_QGadget_Macro<T>::Value};
};

template <typename T>
using stripped_this = typename std::decay<typename std::remove_pointer<T>::type>::type;

template<class... T>
void Unused(T...) noexcept {};

template <typename Container>
using decayed_val_t = typename std::decay<typename std::remove_pointer<typename Container::iterator::value_type>::type>::type;

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

template<typename T, class=void> struct is_container : std::false_type {};
template<typename T>
struct is_container<T, std::void_t<typename T::iterator>> : std::true_type {};
template <typename Container>
using container_val_t = typename Container::iterator::value_type;

template <typename Container>
using container_key_t = typename std::decay<decltype(std::declval<Container>().key(std::declval<container_val_t<Container>>()))>::type;

template <typename Container, bool isContainer>
struct ContainerInfoImpl;


template <typename Container>
struct ContainerInfoImpl<Container, false> {
    using iter_t = void;
    using value_t = void;
    enum Methods {
        has_push_back = false,
        has_append = false,
        has_insert_one_arg = false,
        has_key = false,
        has_value = false,
        has_subscript = false,
        has_contains = false,
        has_contains_key = false,
    };
};

template <typename Container>
struct ContainerInfoImpl<Container, true> {
    using iter_t = typename Container::iterator;
    using value_t = container_val_t<Container>;
private:
    template<typename> static std::false_type has_push_back_impl(...);
    template<typename T> static auto has_push_back_impl(int) ->
        decltype(std::declval<T>().push_back(std::declval<value_t>()), std::true_type());

    template<typename> static std::false_type has_append_impl(...);
    template<typename T> static auto has_append_impl(int) ->
        decltype(std::declval<T>().append(std::declval<value_t>()), std::true_type());

    template<typename> static std::false_type has_insert_impl(...);
    template<typename T> static auto has_insert_impl(int) ->
        decltype(std::declval<T>().insert(std::declval<value_t>()), std::true_type());

    template<typename> static std::false_type has_key_impl(...);
    template<typename T> static auto has_key_impl(int) ->
        decltype(std::declval<T>().key(std::declval<value_t>()), std::true_type());

    template<typename> static std::false_type has_value_impl(...);
    template<typename T> static auto has_value_impl(int) ->
        decltype(std::declval<T>().value({}), std::true_type());

    template<typename> static std::false_type has_subscript_impl(...);
    template<typename T> static auto has_subscript_impl(int) ->
        decltype(std::declval<T>()[0], std::true_type());

    template<typename> static std::false_type has_contains_impl(...);
    template<typename T> static auto has_contains_impl(int) ->
        decltype(std::declval<T>().contains({}), std::true_type());
public:
    enum Methods {
        has_push_back = decltype(has_push_back_impl<Container>(0))::value,
        has_append = decltype(has_append_impl<Container>(0))::value,
        has_insert_one_arg = decltype(has_insert_impl<Container>(0))::value,
        has_key = decltype(has_key_impl<Container>(0))::value,
        has_value = decltype(has_value_impl<Container>(0))::value,
        has_subscript = decltype(has_subscript_impl<Container>(0))::value,
        has_contains = decltype(has_contains_impl<Container>(0))::value,
        has_contains_key = has_key && decltype(has_contains_impl<Container>(0))::value,
    };
};

template <typename Container>
struct ContainerInfo : public ContainerInfoImpl<Container, is_container<Container>::value> {
    using MethodsImpl = typename ContainerInfoImpl<Container, is_container<Container>::value>::Methods;
    using typename ContainerInfoImpl<Container, is_container<Container>::value>::iter_t;
    using typename ContainerInfoImpl<Container, is_container<Container>::value>::value_t;
    enum Info {
        IsContainer = is_container<Container>::value,
    };
    enum Methods {
        has_push_back= MethodsImpl::has_push_back,
        has_append= MethodsImpl::has_append,
        has_insert_one_arg= MethodsImpl::has_insert_one_arg,
        has_key= MethodsImpl::has_key,
        has_value = MethodsImpl::has_value,
        has_subscript = MethodsImpl::has_subscript,
        has_contains = MethodsImpl::has_contains,
        has_contains_key = MethodsImpl::has_contains_key,
    };
};


template <typename T>
struct LambdaInfo
    : public LambdaInfo<decltype(&T::operator())>
{};

// for pointers to member function
template <typename ClassType, typename ReturnType, typename... Args>
struct LambdaInfo<ReturnType(ClassType::*)(Args...) const> {
    enum { ArgumentCount = sizeof...(Args) };
    using Signature = ReturnType(ClassType::*)(Args...) const;
    typedef std::function<ReturnType (Args...)> AsFunction;
};

// for pointers to member function
template <typename ClassType, typename ReturnType, typename... Args>
struct LambdaInfo<ReturnType(ClassType::*)(Args...) > {
    enum { ArgumentCount = sizeof...(Args) };
    using Signature = ReturnType(ClassType::*)(Args...);
    typedef std::function<ReturnType (Args...)> AsFunction;
};

// for function pointers
template <typename ReturnType, typename... Args>
struct LambdaInfo<ReturnType (*)(Args...)>  {
    enum { ArgumentCount = sizeof...(Args) };
    using Signature = ReturnType (*)(Args...);
    typedef std::function<ReturnType (Args...)> AsFunction;
};

template<class Holder, typename = void>
struct is_smart_ptr : std::false_type {};

template <typename T, typename Ret = void>
using enable_if_ptr_typedef = typename std::enable_if<std::is_pointer<typename T::pointer>::value, Ret>::type;

template<class Holder>
struct is_smart_ptr<Holder, std::void_t<
        typename Holder::value_type,
        typename Holder::Type,
        enable_if_ptr_typedef<Holder>
     >> : std::true_type {};

template <class Holder, class Ret = void>
using enable_if_smart_ptr = typename std::enable_if<is_smart_ptr<Holder>::value, Ret>::type;
template <class T, class Ret = void>
using enable_if_ptr = typename std::enable_if<std::is_pointer<T>::value, Ret>::type;
template <class T, class Ret = void>
using enable_if_not_ptr = typename std::enable_if<!std::is_pointer<T>::value && !is_smart_ptr<T>::value, Ret>::type;

}
#endif // METAPROGRAMMING_HPP
