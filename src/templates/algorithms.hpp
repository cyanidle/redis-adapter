#ifndef RADAPTER_CONTAINERS_HPP
#define RADAPTER_CONTAINERS_HPP

#include "private/global.h"
#include "metaprogramming.hpp"
#include "templates/containerfilter.hpp"
#include "templates/enumerator.hpp"
#include "templates/reverser.hpp"
#include "zipiterator.hpp"

namespace Radapter{

template <typename Object, typename Method, typename...Args>
enable_if_not_ptr<Object, typename MethodInfo<Method>::ReturnType> test(const Object &val, Method pred, Args&&...args) {
    return (val.*pred)(std::forward<Args>(args)...);
}

template <typename Object, typename Method, typename...Args>
enable_if_ptr<Object, typename MethodInfo<Method>::ReturnType> test(const Object &val, Method pred, Args&&...args) {
    return (val->*pred)(std::forward<Args>(args)...);
}

template <typename Function, typename...Args>
typename FuncInfo<Function>::ReturnType test(Function pred, Args&&...args) {
    return pred(std::forward<Args>(args)...);
}

template <typename Object, typename Method, typename...Args>
typename std::enable_if<
is_iterator<Object>::value &&
!is_smart_ptr<Object>::value &&
!std::is_pointer<Object>::value,
typename MethodInfo<Method>::ReturnType>::type test(const Object &val, Method pred, Args&&...args) {
    return test(val.value(), pred, std::forward<Args>(args)...);
}

template <typename Object, typename Method, typename...Args>
enable_if_smart_ptr<Object, typename MethodInfo<Method>::ReturnType> test(const Object &val, Method pred, Args&&...args) {
    return test(val.data(), pred, std::forward<Args>(args)...);
}


template<typename Predicate>
struct ContainerTester {
    ContainerTester(Predicate pred) : m_pred(pred) {}
    template <typename ValueT, typename...Args>
    auto operator()(const ValueT &val, Args&&...args) const -> typename MethodInfo<Predicate>::ReturnType {
        return test(val, m_pred, std::forward<Args>(args)...);
    }
private:
    Predicate m_pred;
};

template<typename Predicate>
struct ContainerCompTester {
    ContainerCompTester(Predicate pred) : m_pred(pred) {}
    template <typename ValueT, typename...Args>
    bool operator()(const ValueT &val1, const ValueT &val2, Args&&...args) const {
        return test(val1, m_pred, std::forward<Args>(args)...) < test(val2, m_pred, std::forward<Args>(args)...);
    }
private:
    Predicate m_pred;
};

template <typename Container, typename Predicate>
bool all_of(const Container &container, Predicate predicate) {
    return std::all_of(container.begin(),
                       container.end(),
                       ContainerTester<Predicate>(predicate));
}
template <typename Container, typename Predicate>
bool none_of(const Container &container, Predicate predicate) {
    return std::none_of(container.begin(),
                        container.end(),
                        ContainerTester<Predicate>(predicate));
}
template <typename Container, typename Predicate>
bool any_of(const Container &container, Predicate predicate) {
    return std::any_of(container.begin(),
                       container.end(),
                       ContainerTester<Predicate>(predicate));
}

template <typename Container, typename Predicate>
auto find_if(const Container &container, Predicate predicate) -> typename Container::const_iterator {
    return std::find_if(container.begin(),
                        container.end(),
                        ContainerTester<Predicate>(predicate));
}

template <typename Container, typename Predicate>
auto find_if(Container &container, Predicate predicate) -> typename Container::iterator {
    return std::find_if(container.begin(),
                        container.end(),
                        ContainerTester<Predicate>(predicate));
}

template <typename Container, typename Predicate>
auto for_each(Container &container, Predicate predicate) -> typename Container::iterator {
    return std::for_each(container.begin(),
                        container.end(),
                        ContainerTester<Predicate>(predicate));
}

template <typename Container, typename Predicate>
auto for_each(const Container &container, Predicate predicate) -> typename Container::iterator {
    return std::for_each(container.begin(),
                        container.end(),
                        ContainerTester<Predicate>(predicate));
}

template<typename IterT>
struct SearchResult {
    SearchResult(IterT found, IterT end) : result(found), endIter(end) {}
    IterT result;
    bool wasFound() const {
        return result != endIter;
    }
private:
    IterT endIter;
};

template <typename Container, typename Predicate>
auto max_element(const Container &container, Predicate predicate) -> SearchResult<typename Container::const_iterator> {
    auto found = std::max_element(container.begin(),
                    container.end(),
                    ContainerCompTester<Predicate>(predicate));
    return SearchResult<decltype(found)>(found, container.end());
}

template <typename Container, typename Predicate>
auto max_element(Container &container, Predicate predicate) -> SearchResult<typename Container::iterator> {
    auto found = std::max_element(container.begin(),
                   container.end(),
                   ContainerCompTester<Predicate>(predicate));
    return SearchResult<decltype(found)>(found, container.end());
}

template <typename Container>
auto reverse(const Container &cont) -> ConstReverser<Container>
{
    return ConstReverser<Container>(cont);
}

template <typename Container>
auto reverse(Container &cont) -> Reverser<Container>
{
    return Reverser<Container>(cont);
}

template <typename Target, typename Container, typename Predicate>
struct Accumulator {
    Accumulator(Target &target, Predicate pred) : m_target(target), m_pred(pred) {}
    Target &operator+=(const container_val_t<Container> &val) {
        m_target.*m_pred(val);
        return m_target;
    }
private:
    Target& m_target;
    Predicate m_pred;
};

template <typename Target, typename Container, typename Predicate>
void accumulate(Target &target, const Container &container, Predicate predicate) {
    auto accumulator = Accumulator<Target, Container, Predicate>(target, predicate);
    std::accumulate(container.begin(), container.end(), accumulator);
}

template <typename Container1, typename Container2>
auto zip(Container1 &first, Container2 &second) -> ZipHolder<Container1, Container2>
{
    return ZipHolder<Container1, Container2>(first, second);
}

template <typename Container1, typename Container2>
auto zip(const Container1 &first, const Container2 &second) -> ZipConstHolder<Container1, Container2>
{
    return ZipConstHolder<Container1, Container2>(first, second);
}

template <typename Container, typename CounterType = quint32>
auto enumerate(Container &container) -> EnumeratorHolder<Container, CounterType>
{
    return EnumeratorHolder<Container, CounterType>(container);
}

template <typename Container, typename CounterType = quint32>
auto enumerate(const Container &container) -> ConstEnumeratorHolder<Container, CounterType>
{
    return ConstEnumeratorHolder<Container, CounterType>(container);
}

template <typename Container, typename Filter>
auto filter(Container &container, Filter filter) -> FilterHolder<Container, Filter>
{
    return FilterHolder<Container, Filter>(container, filter);
}

template <typename Container, typename Filter>
auto filter(const Container &container, Filter filter) -> FilterConstHolder<Container, Filter>
{
    return FilterConstHolder<Container, Filter>(container, filter);
}

template<typename Func>
auto as_function(Func&&func) -> typename std::enable_if<FuncInfo<Func>::IsFunction, std::function<typename FuncInfo<Func>::Signature>>::type
{
    return std::function<typename FuncInfo<Func>::Signature>(std::forward<Func>(func));
}

template<typename Lambda>
auto as_function(Lambda&& lambda) -> typename std::enable_if<!FuncInfo<Lambda>::IsFunction, typename LambdaInfo<Lambda>::AsFunction>::type
{
    return typename LambdaInfo<Lambda>::AsFunction(lambda);
}

}
#endif
