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
    inline explicit JsonDict(const QVariant& src, const QString &separator = ":", bool nest = true);
    inline JsonDict(const QVariantMap& src = {}, const QString &separator = ":", bool nest = true);
    inline JsonDict(std::initializer_list<std::pair<QString, QVariant>> initializer);
    explicit inline JsonDict(std::initializer_list<std::pair<QString, JsonDict>> initializer);
    explicit inline JsonDict(QVariantMap&& src, const QString &separator = ":", bool nest = true);
    //! \warning Implicitly covertible to QVariant and QVariantMap
    operator const QVariantMap&() const&;
    operator QVariantMap&() &;
    operator QVariantMap&&() &&;
    QVariant toVariant() const;
    //! Функция доступа к вложенным элементам.
    /// \warning Попытка доступа к несуществующему ключу создает пустое значение в нем,
    /// не повлияет на данные, но стоит быть внимательным (возвращает QVariant& доступный для модификации)
    inline QVariant& operator[](const QStringList& akey);
    inline QVariant& operator[](const QString& akey);
    inline void insert(const QStringList& akey, const JsonDict &value);
    inline void insert(const QStringList& akey, const QVariant &value);
    inline void insert(const QStringList& akey, const QVariantMap &value);
    inline void insert(const QString& akey, const QVariantMap &value);
    //! Move optimisations
    inline void insert(const QStringList& akey, JsonDict &&value);
    inline void insert(const QString& akey, const QVariant &value);
    inline void insert(const QStringList& akey, QVariant &&value);
    inline void insert(const QString& akey, QVariant &&value);
    inline void insert(const QStringList& akey, QVariantMap &&value);
    inline void insert(const QString& akey, QVariantMap &&value);
    inline size_t depth() const;
    void swap(QVariantMap &dict) noexcept;
    inline const QVariant operator[](const QStringList& akey) const;
    inline const QVariant operator[](const QString& akey) const;
    //! Не создает веток по несуществующим ключам
    inline bool isValid(const QStringList& akey) const;
    inline bool isValid(const QString& akey) const;
    inline QList<QVariant> values() const;
    inline const QVariant value(const QString& akey, const QVariant &adefault = QVariant()) const;
    inline const QVariant value(const QStringList& akey, const QVariant &adefault = QVariant()) const;
    //! Оператор глубокого сравнения словарей
    inline bool operator==(const JsonDict& src) const;
    inline bool operator!=(const JsonDict& src) const;
    inline int count() const;
    inline int deepCount() const;
    //! Конвертация в QJsonObject
    inline QJsonObject toJsonObj() const;
    inline QByteArray toBytes(QJsonDocument::JsonFormat format = QJsonDocument::Compact) const;
    //! Заполнение из QJsonObject
    inline static JsonDict fromJsonObj(const QJsonObject &json);
    inline static JsonDict fromJson(const QByteArray &json, QJsonParseError *err = nullptr);
    inline bool contains(const QString &key) const;
    inline bool contains(const QStringList &key) const;
    inline bool contains(const JsonDict &src) const;
    inline static qint64 toIndex(const QString &key);
    inline QStringList firstKey() const;
    inline QVariant &first();
    inline const QVariant &first() const;
    inline QStringList keys(const QString &separator = ":") const;
    inline QVariantMap &top();
    inline const QVariantMap &top() const;
    inline int remove(const QStringList &akey);
    inline QVariant take(const QStringList &akey);
    inline QVariant take(const QString &akey);
    inline bool isEmpty() const;
    inline JsonDict &nest(QChar separator = ':');
    inline JsonDict &nest(const QString &separator);
    inline JsonDict &merge(const JsonDict &src, bool overwrite = true);
    inline JsonDict nest(const QString &separator) const;
    inline JsonDict merge(const JsonDict &src, bool overwrite = true) const;
    inline QVariantMap flatten(const QString &separator = ":") const;
    struct iterator;
    struct const_iterator;
    template <typename MapT>
    struct iterator_base;
    inline JsonDict::iterator begin();
    inline JsonDict::iterator end();
    inline JsonDict::const_iterator begin() const;
    inline JsonDict::const_iterator end() const;
    inline JsonDict::const_iterator cbegin() const;
    inline JsonDict::const_iterator cend() const;
    inline QString printDebug() const;
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
    inline const QVariant *recurseTo(const QStringList &fullKey, int ignoreLastKeys = 0) const;
    inline QVariant *recurseTo(const QStringList &fullKey, int ignoreLastKeys = 0);
    static const QVariant *find(const QVariantMap *dict, const QString &key);
    static QVariant *find(QVariantMap *dict, const QString &key);
    static const QVariant *find(const QVariantList *list, qint64 index);
    static QVariant *find(QVariantList *list, qint64 index);
private:
    inline QString processWarn(const QStringList &src, const int& index);
};

template <typename MapT>
struct JsonDict::iterator_base {
    using map_iter = typename std::conditional<std::is_const<MapT>::value, QVariantMap::const_iterator, QVariantMap::iterator>::type;
    using list_iter = typename std::conditional<std::is_const<MapT>::value, QVariantList::const_iterator, QVariantList::iterator>::type;
    using value_type = typename map_iter::value_type;
    using qual_val_t = typename std::conditional<std::is_const<MapT>::value, const value_type, value_type>::type;
    using ListT = typename std::conditional<std::is_const<MapT>::value, const QVariantList, QVariantList>::type;
    QStringList key() const;
    QStringList domainKey() const;
    const QVariantMap *domainMap() const;
    const QVariantList *domainList() const;
    QString field() const;
    int depth() const;
    bool isDomainMap() const;
    bool isDomainList() const;
    qual_val_t &value() const;
    bool operator==(const iterator_base &other) const;
    bool operator!=(const iterator_base &other) const;
    iterator_base &operator++();
    iterator_base operator++(int);
    iterator_base &operator*();
    qual_val_t *operator->();
    iterator_base(map_iter start, map_iter end);
protected:
    constexpr bool isEnd () const noexcept {return m_flags.testFlag(IsEnd);}
    constexpr bool historyEmpty() const noexcept {return m_traverseHistory.isEmpty();}
    constexpr bool isRecursion() const noexcept {return m_flags.testFlag(IsInRecursion);}
    void stopRecurse() {m_flags.setFlag(IsInRecursion, false);}
    void startRecurse() {m_flags.setFlag(IsInRecursion);}
    void findFirst();
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
        qual_val_t &value() const {
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
    QStack<TraverseState> m_traverseHistory{};
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
        return reinterpret_cast<const QVariantMap*>(m_traverseHistory.last().current->data());
    } else {
        return nullptr;
    }
}

template<typename MapT>
const QVariantList *JsonDict::iterator_base<MapT>::domainList() const
{
    if (!historyEmpty() && isDomainList()) {
        return reinterpret_cast<const QVariantList*>(m_traverseHistory.last().current->data());
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
JsonDict::iterator_base<MapT>::qual_val_t &JsonDict::iterator_base<MapT>::value() const
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
JsonDict::iterator_base<MapT>::qual_val_t *JsonDict::iterator_base<MapT>::operator->()
{
    return &value();
}

template<typename MapT>
JsonDict::iterator_base<MapT>::iterator_base(map_iter start, map_iter end) :
    m_current(start),
    m_end(end),
    m_flags(start == end ? IsEnd : 0)
{
    if (!isEnd()) {
        findFirst();
    }
}

template<typename MapT>
void JsonDict::iterator_base<MapT>::findFirst()
{
    auto *currval = &m_current.value();
    while (currval->type() == QVariant::Map || currval->type() == QVariant::List) {
        auto isMap = currval->type() == QVariant::Map;
        auto *asDict = reinterpret_cast<MapT *>(currval->data());
        auto *asList = reinterpret_cast<ListT *>(currval->data());
        if (m_current == m_end) {
            return;
        }
        if (isMap) {
            if (asDict->isEmpty()) {
                currval = &(++m_current).value();
            } else {
                m_traverseHistory.push({m_current, m_end});
                m_current = asDict->begin();
                m_end = asDict->end();
                currval = &m_current.value();
            }
        } else {
            if (asList->isEmpty()) {
                currval = &(++m_current).value();
            } else {
                m_traverseHistory.push({m_current, m_end});
                m_current = asList->begin();
                m_end = asList->end();
                currval = &m_current.value();
            }
        }
    }
}

template<typename MapT>
QString JsonDict::iterator_base<MapT>::field() const {
    return m_current.key();
}

int JsonDict::deepCount() const
{
    int count = 0;
    for (const auto &iter: *this) {
        Q_UNUSED(iter);
        ++count;
    }
    return count;
}

QStringList JsonDict::keys(const QString &separator) const
{
   QStringList result{};
    for (const auto &iter: *this) {
        result.append(iter.key().join(separator));
    }
    return result;
}

inline QVariantMap &JsonDict::top()
{
    return m_dict;
}

inline const QVariantMap &JsonDict::top() const
{
    return m_dict;
}

bool JsonDict::contains(const QString &key) const
{
    return m_dict.contains(key);
}

bool JsonDict::contains(const QStringList &key) const
{
    if (!key.length()) {
        return false;
    }
    if (key.length() == 1 && m_dict.value(key[0]).isValid()) {
        return true;
    }
    return value(key).isValid();
}


bool JsonDict::contains(const JsonDict &src) const
{
    if (count() < src.count()) {
        return false;
    }
    for (auto otherItem = src.begin();
         otherItem != src.end();
         otherItem++)
    {
        auto thisItem = value(otherItem.key());
        if (!thisItem.isValid() || (thisItem != otherItem.value())) {
            return false;
        }
    }
    return true;

}

inline qint64 JsonDict::toIndex(const QString &key)
{
    static QRegExp checker{"^\\[[0-9]*\\]$"};
    bool ok;
    auto result = key.midRef(1, key.length() - 2).toLong(&ok);
    return ok && checker.exactMatch(key) ? result : -1;
}

const QVariant *JsonDict::recurseTo(const QStringList &fullKey, int ignoreLastKeys) const
{
    if (fullKey.isEmpty()) {
        return nullptr;
    }
    const QVariant* current = nullptr;
    const QVariantMap* currentDict = &m_dict;
    const QVariantList* currentList = nullptr;
    bool recursingDict = true;
    for (int i = 0; i < fullKey.size() - ignoreLastKeys; ++i) {
        auto &key = fullKey[i];
        if (recursingDict) {
            current = find(currentDict, key);
        } else {
            current = find(currentList, toIndex(key));
        }
        if (!current) return nullptr;
        if (current->type() == QVariant::Map) {
            recursingDict = true;
            currentDict = reinterpret_cast<const QVariantMap *>(current->data());
        } else if (current->type() == QVariant::List) {
            recursingDict = false;
            currentList = reinterpret_cast<const QVariantList *>(current->data());
        } else if (i < fullKey.size() - ignoreLastKeys - 1){
            return nullptr;
        }
    }
    return current;
}

inline QVariant *JsonDict::recurseTo(const QStringList &fullKey, int ignoreLastKeys)
{
    if (fullKey.isEmpty()) {
        return nullptr;
    }
    QVariant* current = nullptr;
    QVariantMap* currentDict = &m_dict;
    QVariantList* currentList = nullptr;
    bool recursingDict = true;
    for (int i = 0; i < fullKey.size() - ignoreLastKeys; ++i) {
        auto &key = fullKey[i];
        if (recursingDict) {
            current = find(currentDict, key);
        } else {
            current = find(currentList, toIndex(key));
        }
        if (!current) return nullptr;
        if (current->type() == QVariant::Map) {
            recursingDict = true;
            currentDict = reinterpret_cast<QVariantMap *>(current->data());
        } else if (current->type() == QVariant::List) {
            recursingDict = false;
            currentList = reinterpret_cast<QVariantList *>(current->data());
        } else if (i < fullKey.size() - ignoreLastKeys - 1){
            return nullptr;
        }
    }
    return current;
}

inline const QVariant *JsonDict::find(const QVariantMap *dict, const QString &key)
{
    if (dict->contains(key)) {
        auto iter = dict->constBegin();
        auto *found = &iter;
        while (*found != dict->constEnd() && found->key() != key) {
            ++iter;
        }
        if (*found != dict->constEnd()) return &(found->value());
    }
    return nullptr;
}

inline QVariant *JsonDict::find(QVariantMap *dict, const QString &key)
{
    if (dict->contains(key)) {
        return &(*dict)[key];
    }
    return nullptr;
}

inline const QVariant *JsonDict::find(const QVariantList *list, qint64 index)
{
    if (index < 0) return nullptr;
    if (list->size() > index) {
        return &list->at(index);
    }
    return nullptr;
}

inline QVariant *JsonDict::find(QVariantList *list, qint64 index)
{
    if (index < 0) return nullptr;
    if (list->size() > index) {
        return &list->operator[](index);
    }
    return nullptr;
}

JsonDict::iterator JsonDict::begin()
{
    return JsonDict::iterator(m_dict.begin(), m_dict.end());
}

JsonDict::iterator JsonDict::end()
{
    return JsonDict::iterator(m_dict.end(), m_dict.end());
}

JsonDict::const_iterator JsonDict::begin() const
{
    return JsonDict::const_iterator(m_dict.begin(), m_dict.end());
}

JsonDict::const_iterator JsonDict::end() const
{
    return JsonDict::const_iterator(m_dict.end(), m_dict.end());
}

inline JsonDict::const_iterator JsonDict::cbegin() const
{
    return begin();
}

inline JsonDict::const_iterator JsonDict::cend() const
{
    return end();
}

const QVariant JsonDict::operator[](const QStringList& akey) const
{
    return value(akey);
}

inline void JsonDict::insert(const QStringList &akey, const JsonDict &value)
{
    (*this)[akey] = static_cast<const QVariantMap&>(value);
}

const QVariant JsonDict::value(const QStringList& akey, const QVariant &adefault) const
{
    auto ptr = recurseTo(akey);
    if (ptr) {
        return *ptr;
    } else {
        return adefault;
    }
}

int JsonDict::remove(const QStringList &akey)
{
    auto val = recurseTo(akey, 1);
    if (!val || val->type() != QVariant::Map) return 0;
    return reinterpret_cast<QVariantMap*>(val->data())->remove(akey.constLast());
}

QVariant JsonDict::take(const QStringList &akey)
{
    auto val = recurseTo(akey, 1);
    if (!val || val->type() != QVariant::Map) return {};
    return reinterpret_cast<QVariantMap*>(val->data())->take(akey.constLast());
}

QVariantMap JsonDict::flatten(const QString &separator) const
{
    QVariantMap result;
    for (const auto& iter : *this) {
        auto key = iter.key().join(separator);
        auto value = iter.value();
        result.insert(key, value);
    }
    return result;
}

JsonDict& JsonDict::nest(const QString &separator)
{
    JsonDict newState;
    for (auto iter = m_dict.constBegin(); iter != m_dict.constEnd(); ++iter) {
        const auto &val = iter.value();
        QVariant toInsert;
        if (val.type() == QVariant::List) {
            auto actual = QVariantList{};
            for (const auto &subval : *reinterpret_cast<const QVariantList*>(val.data())) {
                if (subval.type() == QVariant::Map) {
                    actual.append(JsonDict(subval, separator).toVariant());
                } else {
                    actual.append(subval);
                }
            }
            toInsert = actual;
        } else {
            toInsert = val;
        }
        newState.insert(iter.key().split(separator), toInsert);
    }
    swap(newState);
    return *this;
}

JsonDict& JsonDict::merge(const JsonDict &src, bool overwrite)
{
    for (auto &iter : src) {
        auto key = iter.key();
        if (overwrite || !contains(key)) {
            insert(key, iter.value());
        }
    }
    return *this;
}

JsonDict JsonDict::nest(const QString &separator) const
{
    JsonDict result = *this;
    return result.nest(separator);
}

JsonDict JsonDict::merge(const JsonDict &src, bool overwrite) const
{
    JsonDict result;
    return result.merge(src, overwrite);
}

QString JsonDict::processWarn(const QStringList &src, const int& index)
{
    QString result;
    result.append(src.constFirst());
    for (int i = 1; i < index;++i){
        result.append(":");
        result.append(src.at(i));
    }
    return result;
}

//! Дает доступ на чтение/запись значения под ключем, разделитель - ":"
QVariant& JsonDict::operator[](const QStringList& akey)
{
    if (akey.isEmpty()) {
        return m_dict[""];
    }
    QVariant* currentVal = &m_dict[akey.constFirst()];
    for (int index = 1; index < (akey.size()); ++index) {
        auto &nextKey =  akey.at(index);
        auto nextIndex = toIndex(nextKey);
        auto indexValid = nextIndex != -1;
        if (currentVal->isValid()) {
            if (currentVal->type() == QVariant::Map) {
                if (indexValid) {
                    throw std::invalid_argument("Access to nested Dict as to List");
                }
                auto asDict = reinterpret_cast<QVariantMap*>(currentVal->data());
                currentVal = &(asDict->operator[](nextKey));
            } else if (currentVal->type() == QVariant::List) {
                if (!indexValid) {
                    throw std::invalid_argument("Access to nested List as to Dict");
                }
                auto asList = reinterpret_cast<QVariantList*>(currentVal->data());
                asList->reserve(nextIndex + 1);
                while (asList->size() <= nextIndex) {
                    asList->append(QVariant{});
                }
                currentVal = &(asList->operator[](nextIndex));
            } else {
                throw std::invalid_argument("Key overlap! Key: " +
                                            processWarn(akey, index).toStdString());
            }
        } else {
            if (!indexValid) {
                currentVal->setValue(QVariantMap{});
                auto asDict = reinterpret_cast<QVariantMap*>(currentVal->data());
                currentVal = &(asDict->operator[](nextKey));
            } else {
                currentVal->setValue(QVariantList{});
                auto asList = reinterpret_cast<QVariantList*>(currentVal->data());
                asList->reserve(nextIndex + 1);
                while (asList->size() <= nextIndex) {
                    asList->append(QVariant{});
                }
                currentVal = &(asList->operator[](nextIndex));
            }
        }
    }
    return *currentVal;
}

QJsonObject JsonDict::toJsonObj() const
{
    return QJsonObject::fromVariantMap(m_dict);
}

inline QByteArray JsonDict::toBytes(QJsonDocument::JsonFormat format) const
{
    return QJsonDocument(toJsonObj()).toJson(format);
}

JsonDict JsonDict::fromJsonObj(const QJsonObject &json)
{
    return JsonDict(json.toVariantMap());
}

inline JsonDict JsonDict::fromJson(const QByteArray &json, QJsonParseError *err)
{
    return JsonDict(QJsonDocument::fromJson(json, err).toVariant().toMap());
}

bool JsonDict::operator==(const JsonDict& src) const
{
    return m_dict == src.m_dict;
}

bool JsonDict::operator!=(const JsonDict& src) const
{
    return m_dict != src.m_dict;
}

QVariant JsonDict::take(const QString &akey)
{
    return m_dict.take(akey);
}
bool JsonDict::isEmpty() const
{
    return m_dict.isEmpty();
}

inline JsonDict &JsonDict::nest(QChar separator)
{
    JsonDict newState;
    for (auto iter = m_dict.constBegin(); iter != m_dict.constEnd(); ++iter) {
        const auto &val = iter.value();
        QVariant toInsert;
        if (val.type() == QVariant::List) {
            auto actual = QVariantList{};
            for (const auto &subval : *reinterpret_cast<const QVariantList*>(val.data())) {
                if (subval.type() == QVariant::Map) {
                    actual.append(JsonDict(subval, separator).toVariant());
                } else {
                    actual.append(subval);
                }
            }
            toInsert = actual;
        } else {
            toInsert = val;
        }
        newState.insert(iter.key().split(separator), toInsert);
    }
    swap(newState);
    return *this;
}

int JsonDict::count() const
{
    return m_dict.count();
}

QStringList JsonDict::firstKey() const
{
    for (auto &iter : *this) {
        return iter.key();
    }
    throw std::invalid_argument("firstKey() on Empty JsonDict!");
}

inline QVariant &JsonDict::first()
{
    for (auto &iter : *this) {
        return iter.value();
    }
    throw std::invalid_argument("first() on Empty JsonDict!");
}

inline const QVariant &JsonDict::first() const
{
    for (auto &iter : *this) {
        return iter.value();
    }
    throw std::invalid_argument("first() on Empty JsonDict!");
}
QVariant& JsonDict::operator[](const QString& akey)
{
    return m_dict[akey];
}
void JsonDict::insert(const QStringList& akey, const QVariant &value)
{
    (*this)[akey] = value;
}
void JsonDict::insert(const QString& akey, const QVariant &value)
{
    m_dict.insert(akey, value);
}
void JsonDict::insert(const QStringList& akey, QVariant &&value)
{
    (*this)[akey] = std::move(value);
}
void JsonDict::insert(const QString& akey, QVariant &&value)
{
    m_dict.insert(akey, std::move(value));
}

inline void JsonDict::insert(const QStringList &akey, const QVariantMap &value)
{
    (*this)[akey] = value;
}

inline void JsonDict::insert(const QString &akey, const QVariantMap &value)
{
    m_dict.insert(akey, value);
}

inline void JsonDict::insert(const QStringList &akey, QVariantMap &&value)
{
    (*this)[akey] = std::move(value);
}

inline void JsonDict::insert(const QString &akey, QVariantMap &&value)
{
    m_dict.insert(akey, std::move(value));
}

inline void JsonDict::insert(const QStringList &akey, JsonDict &&value)
{
    (*this)[akey] = static_cast<QVariantMap&&>(std::move(value));
}

const QVariant JsonDict::operator[](const QString& akey) const
{
    return m_dict.value(akey);
}
bool JsonDict::isValid(const QStringList& akey) const
{
    return value(akey).isValid();
}
bool JsonDict::isValid(const QString& akey) const
{
    return m_dict.value(akey).isValid();
}

inline QList<QVariant> JsonDict::values() const
{
    QList<QVariant> result;
    for (auto &iter : *this) {
        result.append(iter.value());
    }
    return result;
}
const QVariant JsonDict::value(const QString& akey, const QVariant &adefault) const
{
    return m_dict.value(akey, adefault);
}
JsonDict::JsonDict(const QVariant& src, const QString &separator, bool nest) :
    m_dict(src.toMap())
{
    if (!isEmpty() && nest) this->nest(separator);
}
JsonDict::JsonDict(const QVariantMap& src, const QString &separator, bool nest) :
    m_dict(src)
{
    if (!isEmpty() && nest) this->nest(separator);
}

inline JsonDict::JsonDict(std::initializer_list<std::pair<QString, QVariant> > initializer) :
    m_dict(initializer)
{
    this->nest(':');
}

inline JsonDict::JsonDict(std::initializer_list<std::pair<QString, JsonDict> > initializer) :
    m_dict()
{
    for (const auto &pair : initializer) {
        m_dict.insert(pair.first, static_cast<const QVariantMap&>(pair.second));
    }
    this->nest(':');
}

JsonDict::JsonDict(QVariantMap&& src, const QString &separator, bool nest) :
    m_dict(std::move(src))
{
    if (!isEmpty() && nest) this->nest(separator);
}

inline JsonDict::operator const QVariantMap&() const &
{
    return m_dict;
}

inline JsonDict::operator QVariantMap &() &
{
    return m_dict;
}

inline JsonDict::operator QVariantMap &&() &&
{
    return std::move(m_dict);
}

inline QVariant JsonDict::toVariant() const
{
    return static_cast<const QVariantMap&>(*this);
}

size_t JsonDict::depth() const {
    int maxDepth = 0;
    for(auto &iter :*this) {
        if (iter.depth() > maxDepth) maxDepth = iter.depth();
    }
    return maxDepth;
}

inline void JsonDict::swap(QVariantMap &dict) noexcept
{
    m_dict.swap(dict);
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

inline QString JsonDict::printDebug() const
{
    auto result = QStringLiteral("Json {");
    const auto flat = flatten();
    for (auto iter{flat.begin()}; iter != flat.end(); ++iter) {
        result += QStringLiteral("\n\t%1: %2").arg(iter.key(), iter.value().toString());
    }
    result += "\n}";
    return result;
}

inline QDebug operator<<(QDebug dbg, const JsonDict &json)
{
    QDebugStateSaver saver(dbg);
    dbg.noquote() << json.printDebug();
    return dbg;
}

/*! @} */ //JsonDict doxy
Q_DECLARE_METATYPE(JsonDict)
Q_DECLARE_TYPEINFO(JsonDict, Q_MOVABLE_TYPE);

#endif // JsonDict_H
