#ifndef REVERSER_HPP
#define REVERSER_HPP

#include "private/global.h"

template<typename Container>
struct Reverser {
    using iterator = std::reverse_iterator<typename Container::iterator>;
    using const_iterator = std::reverse_iterator<typename Container::const_iterator>;
    Reverser(Container &container) : m_cont(container) {}
    iterator begin() {
        return iterator(m_cont.end());
    }
    iterator end() {
        return iterator(m_cont.begin());
    }
    const_iterator begin() const {
        return iterator(m_cont.end());
    }
    const_iterator end() const {
        return iterator(m_cont.begin());
    }
    Container &m_cont;
};

template<typename Container>
struct ConstReverser {
    using iterator = std::reverse_iterator<typename Container::const_iterator>;
    using const_iterator = std::reverse_iterator<typename Container::const_iterator>;
    ConstReverser(const Container &container) : m_cont(container) {}
    iterator begin() {
        return iterator(m_cont.end());
    }
    iterator end() {
        return iterator(m_cont.begin());
    }
    const_iterator begin() const {
        return iterator(m_cont.end());
    }
    const_iterator end() const {
        return iterator(m_cont.begin());
    }
    const Container &m_cont;
};

#endif // REVERSER_HPP
