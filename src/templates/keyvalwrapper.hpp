#ifndef KEYVALWRAPPER_HPP
#define KEYVALWRAPPER_HPP

#include "private/global.h"

template <typename Container>
struct ConstKeyValWrapper {
    using const_iterator = typename Container::const_key_value_iterator;
    using iterator = const_iterator;
    ConstKeyValWrapper(const Container &map) :
        m_map(map)
    {}
    iterator begin() {
        return m_map.keyValueBegin();
    }
    iterator end() {
        return m_map.keyValueEnd();
    }
    const_iterator begin() const {
        return m_map.constKeyValueBegin();
    }
    const_iterator end() const {
        return m_map.constKeyValueEnd();
    }
    const_iterator cbegin() const {
        return m_map.constKeyValueBegin()();
    }
    const_iterator cend() const {
        return m_map.constKeyValueEnd();
    }
private:
    const Container &m_map;
};

template <typename Container>
struct KeyValWrapper {
    using const_iterator = typename Container::const_key_value_iterator;
    using iterator = typename Container::key_value_iterator;
    KeyValWrapper(Container &map) :
        m_map(map)
    {}
    iterator begin() {
        return m_map.keyValueBegin();
    }
    iterator end() {
        return m_map.keyValueEnd();
    }
    const_iterator begin() const {
        return m_map.constKeyValueBegin();
    }
    const_iterator end() const {
        return m_map.constKeyValueEnd();
    }
    const_iterator cbegin() const {
        return m_map.constKeyValueBegin()();
    }
    const_iterator cend() const {
        return m_map.constKeyValueEnd();
    }
private:
    Container &m_map;
};

#endif // KEYVALWRAPPER_HPP
