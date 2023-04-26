#ifndef KEYVALWRAPPER_HPP
#define KEYVALWRAPPER_HPP

#include "private/global.h"

template <typename K, typename T>
struct ConstKeyValWrapper {
    using const_iterator = typename QMap<K, T>::const_key_value_iterator;
    using iterator = const_iterator;
    ConstKeyValWrapper(const QMap<K, T> &map) :
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
    const QMap<K, T> &m_map;
};

template <typename K, typename T>
struct KeyValWrapper {
    using const_iterator = typename QMap<K, T>::const_key_value_iterator;
    using iterator = typename QMap<K, T>::key_value_iterator;
    KeyValWrapper(QMap<K, T> &map) :
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
    QMap<K, T> &m_map;
};

#endif // KEYVALWRAPPER_HPP
