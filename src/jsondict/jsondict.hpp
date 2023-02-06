#ifndef JsonDict_H
#define JsonDict_H

#include <QObject>
#include <QDateTime>
#include <QJsonDocument>
#include <QStack>
#include <QList>
#include <QVariantMap>
#include <QString>
#include <QThread>
#include <QJsonObject>
#include <QMutex>
#include <QList>
#include "private/global.h"
/*!
 * \defgroup JsonDict JsonDict
 * \ingroup Core
 * \ingroup QVariantMap
 * \brief Основной носитель информации сообщений.
 *
 *  Подразумевает глубоко вложенную структуру. Все запросы формируются рекурсивно, в соответствии с вложенностью JsonDict.
 * 
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
    static constexpr const char defaultSeparator{':'};
    inline explicit JsonDict(const QVariant& src, const QString &separator = ":", bool nest = true);
    inline JsonDict(const QVariantMap& src = QVariantMap{}, const QString &separator = ":", bool nest = true);
    inline JsonDict(std::initializer_list<std::pair<QString, QVariant>> initializer);
    inline JsonDict(std::initializer_list<std::pair<QString, JsonDict>> initializer);
    inline JsonDict(std::initializer_list<std::pair<QString, QVariant>> initializer, const QString &separator);
    inline JsonDict(std::initializer_list<std::pair<QString, JsonDict>> initializer, const QString &separator);
    inline JsonDict(QVariantMap&& src, const QString &separator = ":", bool nest = true);
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
    template <typename Key, typename... KeysLeft>
    QVariant &operator()(const Key &key, KeysLeft... keys);
    template <typename Key>
    QVariant &operator()(const Key &key);
    template <typename Key, typename... KeysLeft>
    const QVariant operator()(const Key &key, KeysLeft... keys) const;
    template <typename Key>
    const QVariant operator()(const Key &key) const;
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
    inline const QString &firstKey() const;
    inline const QString &lastKey() const;
    inline QStringList firstKeyDeep() const;
    inline QVariant &first();
    inline const QVariant &first() const;
    inline QVariant &firstDeep();
    inline const QVariant &firstDeep() const;
    inline QStringList keysDeep(const QString &separator = ":") const;
    inline QStringList topKeys() const;
    inline int remove(const QStringList &akey);
    inline QVariant take(const QStringList &akey);
    inline QVariant take(const QString &akey);
    inline bool isEmpty() const;
    inline JsonDict &nest(const QString &separator = ":");
    inline JsonDict &merge(const JsonDict &src, bool overwrite = true);
    inline JsonDict nest(const QString &separator = ":") const;
    inline JsonDict merge(const JsonDict &src, bool overwrite = true) const;
    inline QVariantMap flatten(const QString &separator = ":") const;
    struct iterator;
    struct const_iterator;
    template <typename Iter, typename MapT>
    struct iterator_base;
    inline JsonDict::iterator begin();
    inline JsonDict::iterator end();
    inline JsonDict::const_iterator begin() const;
    inline JsonDict::const_iterator end() const;
    inline JsonDict::const_iterator cbegin() const;
    inline JsonDict::const_iterator cend() const;
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
    static const QVariant *find(const QVariantMap *dict, const QString &key);
    static QVariant *find(QVariantMap *dict, const QString &key);
private:

    template <typename... KeysLeft>
    static QVariant &fetchInsert(QVariantMap *dict, const QString &key, KeysLeft... keys);
    static QVariant &fetchInsert(QVariantMap *dict, const QString &key);
    inline QString processWarn(const QStringList &src, const int& index);
};

template <typename Iter, typename MapT>
struct JsonDict::iterator_base {
    using value_type = typename Iter::value_type;
    using qual_val_t = typename std::conditional<std::is_const<MapT>::value, const value_type, value_type>::type;
    QStringList key() const;
    QStringList domain() const;
    QString field() const;
    int depth() const;
    qual_val_t &value() const;
    bool operator==(const iterator_base &other) const;
    bool operator!=(const iterator_base &other) const;
    iterator_base &operator++();
    iterator_base operator++(int);
    iterator_base &operator*();
    iterator_base(Iter start, Iter end);
protected:
    constexpr bool isEnd () const noexcept {return m_flags.testFlag(IsEnd);}
    constexpr bool historyEmpty() const noexcept {return m_traverseHistory.isEmpty();}
    constexpr bool isRecursion() const noexcept {return m_flags.testFlag(IsInRecursion);}
    void stopRecurse() {m_flags.setFlag(IsInRecursion, false);}
    void startRecurse() {m_flags.setFlag(IsInRecursion);}
    void findFirst();
private:
    friend JsonDict::const_iterator;
    friend JsonDict::iterator;
    Iter m_current;
    Iter m_end;
    IterFlags m_flags;
    struct TraverseState {
        Iter current;
        Iter end;
    };
    QStack<TraverseState> m_traverseHistory{};
};

struct JsonDict::const_iterator : public iterator_base<QVariantMap::const_iterator, const QVariantMap> {
    using iterator_base::iterator_base;
    using iterator_base::operator!=;
    using iterator_base::operator==;
    using iterator_base::operator*;
    using iterator_base::value;
    using iterator_base::key;
    using iterator_base::depth;
    using iterator_base::domain;
    using iterator_base::field;
    using iterator_base::operator++;
};


struct JsonDict::iterator : public iterator_base<QVariantMap::iterator, QVariantMap> {
    using iterator_base::iterator_base;
    using iterator_base::operator!=;
    using iterator_base::operator==;
    using iterator_base::operator*;
    using iterator_base::value;
    using iterator_base::key;
    using iterator_base::depth;
    using iterator_base::domain;
    using iterator_base::field;
    using iterator_base::operator++;
};


template<typename Iter, typename MapT>
QStringList JsonDict::iterator_base<Iter, MapT>::key() const {
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

template<typename Iter, typename MapT>
QStringList JsonDict::iterator_base<Iter, MapT>::domain() const {
    if (historyEmpty()) {
        return {m_current.key()};
    }
    QStringList result;
    for (auto &state : m_traverseHistory) {
        result.append(state.current.key());
    }
    return result;
}

template<typename Iter, typename MapT>
int JsonDict::iterator_base<Iter, MapT>::depth() const {
    return m_traverseHistory.size();
}

template<typename Iter, typename MapT>
typename
JsonDict::iterator_base<Iter, MapT>::qual_val_t &JsonDict::iterator_base<Iter, MapT>::value() const
{
    return m_current.value();
}

template<typename Iter, typename MapT>
JsonDict::iterator_base<Iter, MapT> &JsonDict::iterator_base<Iter, MapT>::operator++()
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
        m_traverseHistory.push({.current = m_current, .end = m_end});
        auto *asDict = reinterpret_cast<MapT*>(val->data());
        m_current = asDict->begin();
        m_end = asDict->end();
        startRecurse();
        return ++*this;
    }
    stopRecurse();
    return *this;
}

template<typename Iter, typename MapT>
JsonDict::iterator_base<Iter, MapT> JsonDict::iterator_base<Iter, MapT>::operator++(int) {
    auto temp = *this;
    ++*this;
    return temp;
}

template<typename Iter, typename MapT>
bool JsonDict::iterator_base<Iter, MapT>::operator==(const iterator_base &other) const {
    return m_current == other.m_current;
}

template<typename Iter, typename MapT>
bool JsonDict::iterator_base<Iter, MapT>::operator!=(const iterator_base &other) const {
    return m_current != other.m_current;
}

template<typename Iter, typename MapT>
JsonDict::iterator_base<Iter, MapT> &JsonDict::iterator_base<Iter, MapT>::operator*()
{
    return *this;
}

template<typename Iter, typename MapT>
JsonDict::iterator_base<Iter, MapT>::iterator_base(Iter start, Iter end) :
    m_current(start),
    m_end(end),
    m_flags(start == end ? IsEnd : 0)
{
    if (!isEnd()) {
        findFirst();
    }
}

template<typename Iter, typename MapT>
void JsonDict::iterator_base<Iter, MapT>::findFirst()
{
    auto *currval = &m_current.value();
    while (currval->type() == QVariant::Map) {
        auto *asDict = reinterpret_cast<MapT *>(currval->data());
        if (m_current == m_end) {
            return;
        }
        if (asDict->isEmpty()) {
            currval = &(++m_current).value();
        } else {
            m_traverseHistory.push({.current = m_current, .end = m_end});
            m_current = asDict->begin();
            m_end = asDict->end();
            currval = &m_current.value();
        }
    }
}

template<typename Iter, typename MapT>
QString JsonDict::iterator_base<Iter, MapT>::field() const {
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

QStringList JsonDict::keysDeep(const QString &separator) const
{
   QStringList result{};
    for (const auto &iter: *this) {
        result.append(iter.key().join(separator));
    }
    return result;
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
    bool doesContains = true;
    for (auto otherItem = src.begin();
         otherItem != src.end();
         otherItem++)
    {
        auto thisItem = value(otherItem.key());
        if (!thisItem.isValid() || (thisItem != otherItem.value())) {
            doesContains = false;
            break;
        }
    }
    return doesContains;

}

template<typename... KeysLeft>
QVariant &JsonDict::fetchInsert(QVariantMap *dict, const QString &key, KeysLeft... keys)
{
    auto &inserted = (*dict)[key];
    if (inserted.isValid()) {
        if (inserted.type() == QVariant::Map) {
            return fetchInsert(reinterpret_cast<QVariantMap*>(inserted.data()), keys...);
        } else {
            throw std::runtime_error("Shadowing Access!");
        }
    } else {
        inserted.setValue(QVariantMap{});
        return fetchInsert(reinterpret_cast<QVariantMap*>(inserted.data()), keys...);
    }
}

template <typename Key, typename... KeysLeft>
QVariant &JsonDict::operator()(const Key &key, KeysLeft... keys)
{
    auto current = find(&m_dict, key);
    if (current) {
        if (!current->isValid()) current->setValue(QVariantMap{});
        if (current->type() == QVariant::Map) {
            return fetchInsert(reinterpret_cast<QVariantMap*>(current->data()), keys...);
        } else {
            throw std::runtime_error("Shadowing Access!");
        }
    } else {
        return fetchInsert(&m_dict, key, keys...);
    }
}

template <typename Key>
QVariant &JsonDict::operator()(const Key &key)
{
    return fetchInsert(&m_dict, key);
}

template <typename Key, typename... KeysLeft>
const QVariant JsonDict::operator()(const Key &key, KeysLeft... keys) const
{
    auto current = find(&m_dict, key);
    if (current) {
        if (!current->isValid()) current->setValue(QVariantMap{});
        if (current->type() == QVariant::Map) {
            return fetchInsert(reinterpret_cast<QVariantMap*>(current->data()), keys...);
        } else {
            throw std::runtime_error("Shadowing Access!");
        }
    } else {
        return fetchInsert(&m_dict, key, keys...);
    }
}

template <typename Key>
const QVariant JsonDict::operator()(const Key &key) const
{
    return m_dict.value(key);
}

const QVariant *JsonDict::recurseTo(const QStringList &fullKey, int ignoreLastKeys) const
{
    const auto* currentDict = &m_dict;
    const QVariant* current = nullptr;
    for (int i = 0; i < fullKey.size() - ignoreLastKeys; ++i) {
        auto &key = fullKey[i];
        auto iter = currentDict->constBegin();
        auto *found = &iter;
        while (*found != currentDict->constEnd() && found->key() != key) {
            ++iter;
        }
        if (*found != currentDict->end()) {
            current = &found->value();
            if (current->type() == QVariant::Map) {
                currentDict = reinterpret_cast<const QVariantMap *>(current->data());
            } else {
                return i == fullKey.size() - ignoreLastKeys - 1 ? current : nullptr;
            }
        } else {
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

inline QVariant &JsonDict::fetchInsert(QVariantMap *dict, const QString &key)
{
    return (*dict)[key];
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
    if (value(akey).isValid()){
        QVariantMap *dictPtr = &m_dict;
        QVariant* dictVar;
        for (int i = 0; i < akey.length() - 1; ++i) {
            dictVar = &dictPtr->operator[](akey[i]);
            if (dictVar->type() == QVariant::Map) {
                dictPtr = reinterpret_cast<QVariantMap*>(dictVar->data());
            } else {
                return 0;
            }
        }
        return dictPtr->remove(akey[akey.length() - 1]);
    }
    return 0;
}

QVariant JsonDict::take(const QStringList &akey)
{
    if (value(akey).isValid()){
        QVariantMap *dictPtr = &m_dict;
        QVariant* dictVar;
        for (int i = 0; i < akey.length() - 1; ++i) {
            dictVar = &dictPtr->operator[](akey[i]);
            if (dictVar->type() == QVariant::Map) {
                dictPtr = reinterpret_cast<QVariantMap*>(dictVar->data());
            } else {
                return 0;
            }
        }
        return dictPtr->take(akey[akey.length() - 1]);
    }
    return QVariant();
}

QVariantMap JsonDict::flatten(const QString &separator) const
{
    QVariantMap result;
    for (const auto& iter : *this) {
        result.insert(iter.key().join(separator), iter.value());
    }
    return result;
}

JsonDict& JsonDict::nest(const QString &separator)
{
    JsonDict newState;
    for (auto iter = m_dict.constBegin(); iter != m_dict.constEnd(); ++iter) {
        newState.insert(iter.key().split(separator), iter.value());
    }
    m_dict.swap(newState);
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
    JsonDict result;
    for (auto iter = m_dict.constBegin(); iter != m_dict.constEnd(); ++iter) {
        result.insert(iter.key().split(separator), iter.value());
    }
    return result;
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
    QVariantMap* currentDict = &m_dict;
    QVariant* currentVal = &(currentDict->operator[](akey.constFirst()));
    for (int index = 1; index < (akey.size()); ++index) {
        if (currentVal->isValid()) {
            if (currentVal->type() == QVariant::Map) {
                currentDict = reinterpret_cast<QVariantMap*>(currentVal->data());
                currentVal = &(currentDict->operator[](akey.at(index)));
            } else {
                throw std::runtime_error(std::string("Key overlap! Key: ") +
                                         processWarn(akey, index).toStdString());
            }
        } else {
             currentVal->setValue(QVariantMap{});
             currentDict = reinterpret_cast<QVariantMap*>(currentVal->data());
             currentVal = &(currentDict->operator[](akey.at(index)));
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
const QString &JsonDict::firstKey() const
{
    return m_dict.firstKey();
}
const QString &JsonDict::lastKey() const
{
    return m_dict.lastKey();
}

inline QStringList JsonDict::firstKeyDeep() const
{
    for (auto &iter : *this) {
        return iter.key();
    }
    throw std::invalid_argument("firstKeyDeep() on Empty JsonDict!");
}

QVariant &JsonDict::first()
{
    return m_dict.first();
}
int JsonDict::count() const
{
    return m_dict.count();
}
const QVariant &JsonDict::first() const
{
    return m_dict.first();
}

inline QVariant &JsonDict::firstDeep()
{
    for (auto &iter : *this) {
        return iter.value();
    }
    throw std::invalid_argument("firstDeep() on Empty JsonDict!");
}

inline const QVariant &JsonDict::firstDeep() const
{
    for (auto &iter : *this) {
        return iter.value();
    }
    throw std::invalid_argument("firstDeep() on Empty JsonDict!");
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
    this->nest(":");
}

inline JsonDict::JsonDict(std::initializer_list<std::pair<QString, JsonDict> > initializer) :
    m_dict()
{
    for (const auto &pair : initializer) {
        m_dict.insert(pair.first, static_cast<const QVariantMap&>(pair.second));
    }
    this->nest(":");
}
JsonDict::JsonDict(std::initializer_list<std::pair<QString, QVariant>> initializer, const QString &separator) :
    m_dict(initializer)
{
    if (!isEmpty()) nest(separator);
}

JsonDict::JsonDict(std::initializer_list<std::pair<QString, JsonDict>> initializer, const QString &separator) :
    m_dict()
{
    for (const auto &pair : initializer) {
        m_dict.insert(pair.first, static_cast<const QVariantMap&>(pair.second));
    }
    if (!isEmpty()) this->nest(separator);
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

QStringList JsonDict::topKeys() const {
    return m_dict.keys();
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

/*! @} */ //JsonDict doxy
Q_DECLARE_METATYPE(JsonDict)
Q_DECLARE_TYPEINFO(JsonDict, Q_MOVABLE_TYPE);

#endif // JsonDict_H
