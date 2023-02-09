#ifndef CONTAINERFILTER_HPP
#define CONTAINERFILTER_HPP

#include <iterator>
#include <type_traits>
#include "metaprogramming.hpp"

namespace Radapter {
template <typename Container, typename Filter>
struct FilterIter {
    using iterator_category = std::forward_iterator_tag;
    using iter_t = typename Container::iterator;
    using value_type = typename iter_t::value_type;
    FilterIter(iter_t iter, iter_t end, Filter filter) :
        m_iter(iter),
        m_end(end),
        m_filter(filter)
    {
        if (iter == end) return;
        if (!apply<Filter>(*iter)) {
            ++*this;
        }
    }
    FilterIter& operator++() {
        ++m_iter;
        while(m_iter != m_end) {
            if (apply<Filter>(*m_iter)) {
                break;
            } else {
                ++m_iter;
            }
        }
        return *this;
    }
    FilterIter& operator++(int) {
        auto temp = *this;
        ++*this;
        return temp;
    }
    value_type &value() {
        return *m_iter;
    }
    value_type &operator*() {
        return *m_iter;
    }
    value_type *operator->() {
        return &(*m_iter);
    }
    bool operator==(const FilterIter &other) const {
        return m_iter == other.m_iter;
    }
    bool operator!=(const FilterIter &other) const {
        return m_iter != other.m_iter;
    }
protected:
    template<typename T>
    typename std::enable_if<MethodInfo<T>::IsMethod &&
    (std::is_pointer<value_type>::value || is_iterator<T>::value), bool>::type
    apply(const value_type& val) const {
        return (val->*m_filter)();
    }
    template<typename T>
    typename std::enable_if<MethodInfo<T>::IsMethod && !is_iterator<T>::value, bool>::type
    apply(const value_type& val) const {
        return (val.*m_filter)();
    }
    template<typename T>
    typename std::enable_if<!MethodInfo<T>::IsMethod, bool>::type
    apply(const value_type& val) const {
        return m_filter(val);
    }
private:
    iter_t m_iter;
    iter_t m_end;
    Filter m_filter;
};

template <typename Container, typename Filter>
struct FilterConstIter {
    using iterator_category = std::forward_iterator_tag;
    using iter_t = typename Container::const_iterator;
    using value_type = typename iter_t::value_type;
    FilterConstIter(iter_t iter, iter_t end, Filter filter) :
        m_iter(iter),
        m_end(end),
        m_filter(filter)
    {
        if (iter == end) return;
        if (!apply<Filter>(*iter)) {
            ++*this;
        }
    }
    FilterConstIter& operator++() {
        ++m_iter;
        while(m_iter != m_end) {
            if (apply<Filter>(*m_iter)) {
                break;
            } else {
                ++m_iter;
            }
        }
        return *this;
    }
    FilterConstIter& operator++(int) {
        auto temp = *this;
        ++*this;
        return temp;
    }
    const value_type &operator*() {
        return *m_iter;
    }
    const value_type *operator->() {
        return &(*m_iter);
    }
    const value_type &value() {
        return *m_iter;
    }
    bool operator==(const FilterConstIter &other) const {
        return m_iter == other.m_iter;
    }
    bool operator!=(const FilterConstIter &other) const {
        return m_iter != other.m_iter;
    }
protected:
    template<typename T>
    typename std::enable_if<MethodInfo<T>::IsMethod, bool>::type
    apply(const value_type& val) const {
        return (val.*m_filter)();
    }
    template<typename T>
    typename std::enable_if<MethodInfo<T>::IsMethod &&
     (std::is_pointer<value_type>::value || is_iterator<T>::value), bool>::type
    apply(const value_type& val) const {
        return (val->*m_filter)();
    }
    template<typename T>
    typename std::enable_if<!MethodInfo<T>::IsMethod && !is_iterator<T>::value, bool>::type
    apply(const value_type& val) const {
        return m_filter(val);
    }
private:
    iter_t m_iter;
    iter_t m_end;
    Filter m_filter;
};


template <typename Container, typename Filter>
struct FilterHolder {
    using iterator = FilterIter<Container, Filter>;
    using const_iterator = FilterConstIter<Container, Filter>;
    FilterHolder(Container *first, Filter filter) :
        m_cont(first),
        m_filter(filter)
    {}
    bool nonePass() const {
        return begin() == end();
    }
    bool anyPass() const {
        return !nonePass();
    }
    size_t size() const {
        size_t result{0};
        for (auto iter{begin()}; iter != end(); ++iter, ++result) {}
        return result;
    }
    iterator begin() {
        return iterator(m_cont->begin(), m_cont->end(), m_filter);
    }
    iterator end() {
        return iterator(m_cont->end(), m_cont->end(), m_filter);
    }
    const_iterator begin() const {
        return const_iterator(m_cont->cbegin(), m_cont->cend(), m_filter);
    }
    const_iterator end() const {
        return const_iterator(m_cont->cend(), m_cont->cend(), m_filter);
    }
    const_iterator cbegin() const {
        return const_iterator(m_cont->begin(), m_cont->end(), m_filter);
    }
    const_iterator cend() const {
        return const_iterator(m_cont->end(), m_cont->end(), m_filter);
    }
private:
    Container *m_cont;
    Filter m_filter;
};


template <typename Container, typename Filter>
struct FilterConstHolder {
    using const_iterator = FilterConstIter<Container, Filter>;
    using iterator = FilterConstIter<Container, Filter>;
    FilterConstHolder(const Container *first, Filter filter) :
        m_cont(first),
        m_filter(filter)
    {}
    bool nonePass() const {
        return begin() == end();
    }
    bool anyPass() const {
        return !nonePass();
    }
    size_t size() const {
        size_t result;
        for (auto iter{begin()}; iter != end(); ++iter, ++result) {}
        return result;
    }
    iterator begin() {
        return iterator(m_cont->begin(), m_cont->end(), m_filter);
    }
    iterator end() {
        return iterator(m_cont->end(), m_cont->end(), m_filter);
    }
    const_iterator begin() const {
        return const_iterator(m_cont->cbegin(), m_cont->cend(), m_filter);
    }
    const_iterator end() const {
        return const_iterator(m_cont->cend(), m_cont->cend(), m_filter);
    }
    const_iterator cbegin() const {
        return const_iterator(m_cont->begin(), m_cont->end(), m_filter);
    }
    const_iterator cend() const {
        return const_iterator(m_cont->end(), m_cont->end(), m_filter);
    }
private:
    const Container *m_cont;
    Filter m_filter;
};

}

#endif // CONTAINERFILTER_HPP
