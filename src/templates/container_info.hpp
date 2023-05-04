#ifndef CONTAINER_INFO_HPP
#define CONTAINER_INFO_HPP

#include "private/global.h"

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

#endif // CONTAINER_INFO_HPP
