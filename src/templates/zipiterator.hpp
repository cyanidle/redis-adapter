#ifndef ZIPITERATOR_HPP
#define ZIPITERATOR_HPP

#include "private/global.h"
#include "metaprogramming.hpp"
#include <QPair>

template <typename Container1, typename Container2>
//! Iterates like Pythons zip() over two containers
struct ZipIterator {
    using iterator_category = std::forward_iterator_tag;
    using first_iter = typename Container1::iterator;
    using second_iter = typename Container2::iterator;
    using first_value_type = typename first_iter::value_type;
    using second_value_type = typename second_iter::value_type;
    using val_refs_pair = QPair<first_value_type&, second_value_type&>;

    ZipIterator(first_iter first, second_iter second) :
        m_first(first),
        m_second(second)
    {}
    ZipIterator& operator++() {
        ++m_first;
        ++m_second;
        return *this;
    }
    ZipIterator& operator++(int) {
        auto temp = *this;
        ++*this;
        return temp;
    }
    inline val_refs_pair operator*() { return value(); }
    val_refs_pair value() {
        return val_refs_pair(firstValue(), secondValue());
    }
    first_value_type &firstValue() {
        return *m_first;
    }
    second_value_type &secondValue() {
        return *m_second;
    }
    bool operator==(const ZipIterator &other) const {
        return m_first == other.m_first &&
               m_second == other.m_second;
    }
    bool operator!=(const ZipIterator &other) const {
        return !(*this==other);
    }
private:
    first_iter m_first;
    second_iter m_second;
};

template <typename Container1, typename Container2>
struct ZipConstIterator {
    using iterator_category = std::forward_iterator_tag;
    using first_iter = typename Container1::const_iterator;
    using second_iter = typename Container2::const_iterator;
    using first_value_type = typename first_iter::value_type;
    using second_value_type = typename second_iter::value_type;
    using val_refs_pair = QPair<const first_value_type&, const second_value_type&>;

    ZipConstIterator(first_iter first, second_iter second) :
        m_first(first),
        m_second(second)
    {}
    ZipConstIterator& operator++() {
        ++m_first;
        ++m_second;
        return *this;
    }
    ZipConstIterator& operator++(int) {
        auto temp = *this;
        ++*this;
        return temp;
    }
    inline val_refs_pair operator*() { return value(); }
    val_refs_pair value() {
        return val_refs_pair(firstValue(), secondValue());
    }
    const first_value_type &firstValue() {
        return *m_first;
    }
    const second_value_type &secondValue() {
        return *m_second;
    }
    bool operator==(const ZipConstIterator &other) const {
        return m_first == other.m_first &&
               m_second == other.m_second;
    }
    bool operator!=(const ZipConstIterator &other) const {
        return !(*this==other);
    }
private:
    first_iter m_first;
    second_iter m_second;
};

template <typename Container1, typename Container2>
struct ZipHolder {
    using const_iterator = ZipConstIterator<Container1, Container2>;
    using iterator = ZipIterator<Container1, Container2>;
    ZipHolder(Container1 &first, Container2 &second) :
        m_firstCont(first),
        m_secondCont(second)
    {}
    iterator begin() {
        return iterator(m_firstCont.begin(), m_secondCont.begin());
    }
    iterator end() {
        auto firstSmaller = m_firstCont.size() <= m_secondCont.size();
        auto minCount = firstSmaller ? m_firstCont.size() : m_secondCont.size();
        return iterator(firstSmaller ? m_firstCont.end() : m_firstCont.begin() + minCount,
                        !firstSmaller ? m_secondCont.end() : m_secondCont.begin() + minCount);
    }
    const_iterator begin() const {
        return cbegin();
    }
    const_iterator end() const {
        return cend();
    }
    const_iterator cbegin() const {
        return iterator(m_firstCont.cbegin(), m_secondCont.cbegin());
    }
    const_iterator cend() const {
        auto firstSmaller = m_firstCont.size() <= m_secondCont.size();
        auto minCount = firstSmaller ? m_firstCont.size() : m_secondCont.size();
        return iterator(firstSmaller ? m_firstCont.cend() : m_firstCont.cbegin() + minCount,
                        !firstSmaller ? m_secondCont.cend() : m_secondCont.cbegin() + minCount);
    }
private:
    Container1 &m_firstCont;
    Container2 &m_secondCont;
};

template <typename Container1, typename Container2>
struct ZipConstHolder {
    using const_iterator = ZipConstIterator<Container1, Container2>;
    using iterator = ZipConstIterator<Container1, Container2>;
    ZipConstHolder(const Container1 &first, const Container2 &second) :
        m_firstCont(first),
        m_secondCont(second)
    {}
    const_iterator begin() const {
        return cbegin();
    }
    const_iterator end() const {
        return cend();
    }
    const_iterator cbegin() const {
        return iterator(m_firstCont.cbegin(), m_secondCont.cbegin());
    }
    const_iterator cend() const {
        auto firstSmaller = m_firstCont.size() <= m_secondCont.size();
        // cend() must return ends which are the same size (if second is bigger, than seconds.begin() + first.size() is used)
        // Iteration stops on shortest container
        auto minCount = firstSmaller ? m_firstCont.size() : m_secondCont.size();
        return iterator(firstSmaller ? m_firstCont.cend() : m_firstCont.cbegin() + minCount,
                        !firstSmaller ? m_secondCont.cend() : m_secondCont.cbegin() + minCount);
    }
private:
    const Container1 &m_firstCont;
    const Container2 &m_secondCont;
};
#endif // ZIPITERATOR_HPP
