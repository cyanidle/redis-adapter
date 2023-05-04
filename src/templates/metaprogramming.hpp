#ifndef METAPROGRAMMING_HPP
#define METAPROGRAMMING_HPP

#include "private/global.h"
#include "has_macros_tests.hpp"
#include "container_info.hpp"
#include "callable_info.hpp"
#include "has_macros_tests.hpp"

namespace Radapter {

template<bool...> struct bool_pack;
template<bool... bs>
using all_true = std::is_same<bool_pack<bs..., true>, bool_pack<true, bs...>>;

template <typename T>
using stripped_this = typename std::decay<typename std::remove_pointer<T>::type>::type;

#define THIS_TYPE ::Radapter::stripped_this<decltype(this)>

template<class... T>
void Unused(T...) noexcept {};

template <typename Container>
using decayed_val_t = typename std::decay<typename std::remove_pointer<typename Container::iterator::value_type>::type>::type;

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

template<class Holder>
constexpr bool is_smart_ptr_v = is_smart_ptr<Holder>::value;

template <class Holder, class Ret = void>
using enable_if_smart_ptr = typename std::enable_if<is_smart_ptr<Holder>::value, Ret>::type;
template <class T, class Ret = void>
using enable_if_ptr = typename std::enable_if<std::is_pointer<T>::value, Ret>::type;
template <class T, class Ret = void>
using enable_if_not_ptr = typename std::enable_if<!std::is_pointer<T>::value && !is_smart_ptr<T>::value, Ret>::type;

}
#endif // METAPROGRAMMING_HPP
