#ifndef ENUMERATOR_HPP
#define ENUMERATOR_HPP

#include <QtGlobal>
namespace Radapter {
template <typename Container, typename CounterType = quint32>
struct Enumerator {
    using counter_t = CounterType;
    using iter_t = typename Container::iterator;
    using value_type = typename iter_t::value_type;
    struct Result {
        counter_t count;
        value_type& value;
    };
    Enumerator(iter_t iter) : m_iter(iter) {}
    Enumerator& operator++() {
        ++m_count;
        ++m_iter;
        return *this;
    }
    Enumerator& operator++(int) {
        auto temp = *this;
        ++*this;
        return temp;
    }
    Result operator *() {
        return Result{.count = count(), .value = value()};
    }
    counter_t count() const {
        return m_count;
    }
    value_type &value() {
        return *m_iter;
    }
    bool operator==(const Enumerator &other) const {
        return m_iter == other.m_iter;
    }
    bool operator!=(const Enumerator &other) const {
        return m_iter != other.m_iter;
    }
private:
    iter_t m_iter;
    counter_t m_count{0};
};

template <typename Container, typename CounterType = quint32>
struct ConstEnumerator {
    using counter_t = CounterType;
    using iter_t = typename Container::const_iterator;
    using value_type = typename iter_t::value_type;
    struct Result {
        counter_t count;
        const value_type& value;
    };
    ConstEnumerator(iter_t iter, counter_t count = 0) : m_iter(iter), m_count(count) {}
    ConstEnumerator& operator++() {
        ++m_count;
        ++m_iter;
        return *this;
    }
    ConstEnumerator& operator++(int) {
        auto temp = *this;
        ++*this;
        return temp;
    }
    Result operator *() const {
        return Result{.count = count(), .value = value()};
    }
    counter_t count() const {
        return m_count;
    }
    const value_type &value() const {
        return *m_iter;
    }
    bool operator==(const ConstEnumerator &other) const {
        return m_iter == other.m_iter;
    }
    bool operator!=(const ConstEnumerator &other) const {
        return m_iter != other.m_iter;
    }
private:
    iter_t m_iter;
    counter_t m_count{0};
};


template <typename Container, typename CounterType = quint32>
struct EnumeratorHolder {
    using const_iterator = ConstEnumerator<Container, CounterType>;
    using iterator = Enumerator<Container, CounterType>;
    EnumeratorHolder(Container &first) :
        m_firstCont(first)
    {}
    iterator begin() {
        return iterator(m_firstCont.begin());
    }
    iterator end() {
        return iterator(m_firstCont.end());
    }
    const_iterator end() const {
        return iterator(m_firstCont.end());
    }
    const_iterator begin() const {
        return iterator(m_firstCont.begin());
    }
    const_iterator cend() const {
        return iterator(m_firstCont.end());
    }
    const_iterator cbegin() const {
        return iterator(m_firstCont.begin());
    }
private:
    Container &m_firstCont;
};


template <typename Container, typename CounterType = quint32>
struct ConstEnumeratorHolder {
    using const_iterator = ConstEnumerator<Container, CounterType>;
    using iterator = ConstEnumerator<Container, CounterType>;
    ConstEnumeratorHolder(const Container &first) :
        m_firstCont(&first)
    {}
    iterator begin() {
        return iterator(m_firstCont->cbegin());
    }
    iterator end() {
        return iterator(m_firstCont->cend(), m_firstCont->size());
    }
    const_iterator end() const {
        return iterator(m_firstCont->end(), m_firstCont->size());
    }
    const_iterator begin() const {
        return iterator(m_firstCont->begin());
    }
    const_iterator cend() const {
        return iterator(m_firstCont->end(), m_firstCont->size());
    }
    const_iterator cbegin() const {
        return iterator(m_firstCont->begin());
    }
private:
    const Container *m_firstCont;
};

}
#endif // ENUMERATOR_HPP
