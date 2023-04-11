#ifndef JsonDict_H
#define JsonDict_H

#include <QObject>
#include <QJsonDocument>
#include <QStack>
#include <QList>
#include <QDebug>
#include <QVariantMap>
#include <QString>
#include <QRegExp>
#include <QJsonObject>
#include <QList>
#include <stdexcept>
#include "private/global.h"
/*!
 * \defgroup JsonDict JsonDict
 * \ingroup Core
 * \ingroup QVariantMap
 * \brief Основной носитель информации сообщений.
 *
 *  Подразумевает глубоко вложенную структуру. Все запросы формируются рекурсивно, в соответствии с вложенностью JsonDict.
 *  Фактически является JsonObj
 * \code 
 * Core::JsonDict{
 * {domain}:
 *      {domain}:
 *           {field}:
 *                value0 // key() = domain, domain, field;  field() = field;  domain() = domain:domain;
 *      {domain}:
 *           {field}:
 *                value1
 * }
 * \endcode
 * 
 *  @{
 */
class JsonDict
{
public:
    JsonDict() = default;
    explicit JsonDict(const QVariant& src, QChar separator = ':', bool nest = true);
    explicit JsonDict(const QVariant& src, const QString &separator, bool nest = true);
    JsonDict(const QVariantMap& src, QChar separator = ':', bool nest = true);
    JsonDict(const QVariantMap& src, const QString &separator, bool nest = true);
    JsonDict(std::initializer_list<std::pair<QString, QVariant>> initializer);
    explicit JsonDict(QVariantMap&& src, const QString &separator = ":", bool nest = true);
    //! \warning Implicitly covertible to QVariant and QVariantMap
    operator const QVariantMap&() const&;
    operator QVariantMap&() &;
    operator QVariantMap&&() &&;
    QVariant toVariant() const;
    //! Функция доступа к вложенным элементам.
    /// \warning Попытка доступа к несуществующему ключу создает пустое значение в нем,
    /// не повлияет на данные, но стоит быть внимательным (возвращает QVariant& доступный для модификации)
    struct iterator;
    struct const_iterator;
    QVariant& operator[](const iterator& akey);
    QVariant& operator[](const const_iterator& akey);
    QVariant& operator[](const QStringList& akey);
    QVariant& operator[](const QString& akey);
    void insert(const QStringList& akey, const JsonDict &value);
    void insert(const QStringList& akey, const QVariant &value);
    void insert(const QStringList& akey, const QVariantMap &value);
    void insert(const QString& akey, const QVariantMap &value);
    //! Move optimisations
    void insert(const QStringList& akey, JsonDict &&value);
    void insert(const QString& akey, const QVariant &value);
    void insert(const QStringList& akey, QVariant &&value);
    void insert(const QString& akey, QVariant &&value);
    void insert(const QStringList& akey, QVariantMap &&value);
    void insert(const QString& akey, QVariantMap &&value);
    size_t depth() const;
    void swap(QVariantMap &dict) noexcept;
    const QVariant operator[](const QStringList& akey) const;
    const QVariant operator[](const QString& akey) const;
    //! Не создает веток по несуществующим ключам
    bool isValid(const QStringList& akey) const;
    bool isValid(const QString& akey) const;
    QList<QVariant> values() const;
    const QVariant value(const QString& akey, const QVariant &adefault = QVariant()) const;
    const QVariant value(const QStringList& akey, const QVariant &adefault = QVariant()) const;
    //! Оператор глубокого сравнения словарей
    bool operator==(const JsonDict& src) const;
    bool operator!=(const JsonDict& src) const;
    int count() const;
    int deepCount() const;
    //! Конвертация в QJsonObject
    QJsonObject toJsonObj() const;
    QByteArray toBytes(QJsonDocument::JsonFormat format = QJsonDocument::Compact) const;
    //! Заполнение из QJsonObject
    static JsonDict fromJsonObj(const QJsonObject &json);
    static JsonDict fromJson(const QByteArray &json, QJsonParseError *err = nullptr);
    bool contains(const QString &key) const;
    bool contains(const QStringList &key) const;
    bool contains(const JsonDict &src) const;
    static qint64 toIndex(const QString &key);
    QStringList firstKey() const;
    QVariant &first();
    const QVariant &first() const;
    QStringList keys(const QString &separator = ":") const;
    QVariantMap &top();
    const QVariantMap &top() const;
    int remove(const QStringList &akey);
    QVariant take(const QStringList &akey);
    QVariant take(const QString &akey);
    bool isEmpty() const;
    JsonDict diff(const JsonDict &other) const;
    JsonDict &nest(QChar separator = ':');
    JsonDict &nest(const QString &separator);
    JsonDict nest(const QString &separator) const;
    JsonDict &operator+=(const JsonDict &src);
    JsonDict operator+(const JsonDict &src) const;
    JsonDict operator-(const JsonDict &src) const;
    JsonDict &operator-=(const JsonDict &src);
    JsonDict &merge(const JsonDict &src, bool overwrite = true);
    JsonDict merge(const JsonDict &src) const;
    QVariantMap flatten(const QString &separator = ":") const;
    template <typename MapT>
    struct iterator_base;
    JsonDict::iterator begin();
    JsonDict::iterator end();
    JsonDict::const_iterator begin() const;
    JsonDict::const_iterator end() const;
    JsonDict::const_iterator cbegin() const;
    JsonDict::const_iterator cend() const;
    QString printDebug() const;
    friend QDebug operator<<(QDebug dbg, const JsonDict &json);
protected:
    friend struct iterator;
    friend struct const_iterator;
    enum IteratorFlagValues {
        None = 0,
        IsEnd = 1 << 1,
        IsInRecursion = 1 << 2,
    };
    Q_DECLARE_FLAGS(IterFlags, IteratorFlagValues);
    QVariantMap m_dict;
    const QVariant *recurseTo(const QStringList &fullKey, int ignoreLastKeys = 0) const;
    QVariant *recurseTo(const QStringList &fullKey, int ignoreLastKeys = 0);
    static const QVariant *find(const QVariantMap *dict, const QString &key);
    static QVariant *find(QVariantMap *dict, const QString &key);
    static const QVariant *find(const QVariantList *list, qint64 index);
    static QVariant *find(QVariantList *list, qint64 index);
private:
    QString processWarn(const QStringList &src, const int& index);
};

template <typename MapT>
struct JsonDict::iterator_base {
    using map_iter = typename std::conditional<std::is_const<MapT>::value, QVariantMap::const_iterator, QVariantMap::iterator>::type;
    using list_iter = typename std::conditional<std::is_const<MapT>::value, QVariantList::const_iterator, QVariantList::iterator>::type;
    using value_type = typename map_iter::value_type;
    template <typename T>
    using qual_val = typename std::conditional<std::is_const<MapT>::value, const T, T>::type;
    using qual_map_value = typename std::conditional<std::is_const<MapT>::value, const value_type, value_type>::type;
    using ListT = typename std::conditional<std::is_const<MapT>::value, const QVariantList, QVariantList>::type;
    QStringList key() const;
    QStringList domainKey() const;
    const QVariantMap *domainMap() const;
    const QVariantList *domainList() const;
    QString field() const;
    int depth() const;
    bool isDomainMap() const;
    bool isDomainList() const;
    qual_map_value &value() const;
    template <typename T>
    qual_val<T> &value() const {return value().template value<T>();}
    bool operator==(const iterator_base &other) const;
    bool operator!=(const iterator_base &other) const;
    iterator_base &operator++();
    iterator_base operator++(int);
    iterator_base &operator*();
    qual_map_value *operator->();
    iterator_base(map_iter start, map_iter end);
protected:
    constexpr bool isEnd () const noexcept {return m_flags.testFlag(IsEnd);}
    constexpr bool historyEmpty() const noexcept {return m_traverseHistory.isEmpty();}
    constexpr bool isRecursion() const noexcept {return m_flags.testFlag(IsInRecursion);}
    void stopRecurse() {m_flags.setFlag(IsInRecursion, false);}
    void startRecurse() {m_flags.setFlag(IsInRecursion);}
private:
    struct NestedIter {
        NestedIter() : is_valid(false) {}
        NestedIter(list_iter list) : is_map(false), list(list) {}
        NestedIter(map_iter map) : is_map(true), map(map) {}
        NestedIter(const NestedIter& other) : is_map(other.is_map), count(other.count) {
            if (is_map) {
                map = other.map;
            } else {
                list = other.list;
            }
        }
        QString key() const {
            assert(is_valid);
            return is_map ? map.key() : QStringLiteral("[%1]").arg(count);
        }
        qual_map_value &value() const {
            assert(is_valid);
            return is_map ? map.value() : *list;
        }
        NestedIter &operator=(const NestedIter &other) {
            is_valid = other.is_valid;
            is_map = other.is_map;
            count = other.count;
            if (is_map) map = other.map;
            else list = other.list;
            return *this;
        }
        bool operator==(const NestedIter &other) const {
            assert(is_valid);
            if (is_map != other.is_map) return false;
            return is_map ? map == other.map : list == other.list;
        }
        bool operator!=(const NestedIter &other) const {
            return !(*this==other);
        }
        NestedIter &operator++() {
            assert(is_valid);
            if (is_map) {
                ++map;
            } else {
                ++count;
                ++list;
            }
            return *this;
        }
        bool is_map{false};
        bool is_valid{true};
        quint16 count{0};
        union {
            map_iter map;
            list_iter list;
        };
    };
    NestedIter m_current;
    NestedIter m_end;
    IterFlags m_flags;
    struct TraverseState {
        NestedIter current;
        NestedIter end;
    };
    QStack<TraverseState> m_traverseHistory;
    friend JsonDict;
};

struct JsonDict::const_iterator : public iterator_base<const QVariantMap> {
    using iterator_base::iterator_base;
    using iterator_base::operator!=;
    using iterator_base::operator==;
    using iterator_base::operator*;
    using iterator_base::operator->;
    using iterator_base::value;
    using iterator_base::key;
    using iterator_base::depth;
    using iterator_base::domainKey;
    using iterator_base::field;
    using iterator_base::operator++;
    using iterator_base::isDomainMap;
    using iterator_base::isDomainList;
    using iterator_base::domainMap;
    using iterator_base::domainList;
private:
    friend JsonDict;
};


struct JsonDict::iterator : public iterator_base<QVariantMap> {
    using iterator_base::iterator_base;
    using iterator_base::operator!=;
    using iterator_base::operator==;
    using iterator_base::operator*;
    using iterator_base::operator->;
    using iterator_base::value;
    using iterator_base::key;
    using iterator_base::depth;
    using iterator_base::domainKey;
    using iterator_base::field;
    using iterator_base::operator++;
    using iterator_base::isDomainMap;
    using iterator_base::isDomainList;
    using iterator_base::domainMap;
    using iterator_base::domainList;
private:
    friend JsonDict;
};


template<typename MapT>
QStringList JsonDict::iterator_base< MapT>::key() const {
    if (historyEmpty()) {
        return {m_current.key()};
    }
    QStringList result;
    for (auto &state : m_traverseHistory) {
        result.append(state.current.key());
    }
    result.append(m_current.key());
    return result;
}

template<typename MapT>
QStringList JsonDict::iterator_base< MapT>::domainKey() const {
    if (historyEmpty()) {
        return {m_current.key()};
    }
    QStringList result;
    for (auto &state : m_traverseHistory) {
        result.append(state.current.key());
    }
    return result;
}

template<typename MapT>
const QVariantMap *JsonDict::iterator_base<MapT>::domainMap() const
{
    if (!historyEmpty() && isDomainMap()) {
        return reinterpret_cast<const QVariantMap*>(m_traverseHistory.last().current.value().data());
    } else {
        return nullptr;
    }
}

template<typename MapT>
const QVariantList *JsonDict::iterator_base<MapT>::domainList() const
{
    if (!historyEmpty() && isDomainList()) {
        return reinterpret_cast<const QVariantList*>(m_traverseHistory.last().current.value().data());
    } else {
        return nullptr;
    }
}

template<typename MapT>
int JsonDict::iterator_base< MapT>::depth() const {
    return m_traverseHistory.size();
}

template<typename MapT>
bool JsonDict::iterator_base<MapT>::isDomainMap() const
{
    return m_current.is_map;
}

template<typename MapT>
bool JsonDict::iterator_base<MapT>::isDomainList() const
{
    return !isDomainMap();
}

template<typename MapT>
typename
JsonDict::iterator_base<MapT>::qual_map_value &JsonDict::iterator_base<MapT>::value() const
{
    return m_current.value();
}

template<typename MapT>
JsonDict::iterator_base<MapT> &JsonDict::iterator_base<MapT>::operator++()
{
    if (!isRecursion()){
        ++m_current;
    }
    if (m_current == m_end) {
        if (historyEmpty()) {
            stopRecurse();
            return *this;
        }
        auto popped = m_traverseHistory.pop();
        m_current = popped.current;
        m_end = popped.end;
        ++m_current;
        startRecurse();
        return ++*this;
    }
    auto *val = &m_current.value();
    if (val->type() == QVariant::Map) {
        m_traverseHistory.push(TraverseState{m_current, m_end});
        auto *asDict = reinterpret_cast<MapT*>(val->data());
        m_current = asDict->begin();
        m_end = asDict->end();
        startRecurse();
        return ++*this;
    } else if (val->type() == QVariant::List) {
        m_traverseHistory.push(TraverseState{m_current, m_end});
        auto *asList = reinterpret_cast<ListT*>(val->data());
        m_current = asList->begin();
        m_end = asList->end();
        startRecurse();
        return ++*this;
    } else if (!val->isValid()) {
        ++m_current;
        startRecurse();
        return ++*this;
    }
    stopRecurse();
    return *this;
}

template<typename MapT>
JsonDict::iterator_base<MapT> JsonDict::iterator_base<MapT>::operator++(int) {
    auto temp = *this;
    ++*this;
    return temp;
}

template<typename MapT>
bool JsonDict::iterator_base<MapT>::operator==(const iterator_base &other) const {
    return m_current == other.m_current;
}

template<typename MapT>
bool JsonDict::iterator_base<MapT>::operator!=(const iterator_base &other) const {
    return m_current != other.m_current;
}

template<typename MapT>
JsonDict::iterator_base<MapT> &JsonDict::iterator_base<MapT>::operator*()
{
    return *this;
}

template<typename MapT>
typename
JsonDict::iterator_base<MapT>::qual_map_value *JsonDict::iterator_base<MapT>::operator->()
{
    return &value();
}

template<typename MapT>
JsonDict::iterator_base<MapT>::iterator_base(map_iter start, map_iter end) :
    m_current(start),
    m_end(end),
    m_flags(start == end ? IsEnd : 0),
    m_traverseHistory()
{
    if (!isEnd()) {
        startRecurse();
        ++(*this);
    }
}

template<typename MapT>
QString JsonDict::iterator_base<MapT>::field() const {
    return m_current.key();
}

namespace Radapter {
    namespace literals {
        inline JsonDict operator "" _json(const char* str, std::size_t n)
        {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(QByteArray(str, static_cast<int>(n)), &err);
            if (!doc.isObject() || err.error != QJsonParseError::NoError) {
                throw std::runtime_error("Json Parse Error!");
            }
            return JsonDict::fromJsonObj(doc.object());
        }
    }
}
QDebug operator<<(QDebug dbg, const JsonDict &json);

/*! @} */ //JsonDict doxy
Q_DECLARE_METATYPE(JsonDict)
Q_DECLARE_TYPEINFO(JsonDict, Q_MOVABLE_TYPE);

#endif // JsonDict_H
