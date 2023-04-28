#ifndef JSON_DICT_H
#define JSON_DICT_H

#include <QObject>
#include <QJsonDocument>
#include <QStack>
#include <QList>
#include <QDebug>
#include <QVariantMap>
#include <QString>
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
    JsonDict(JsonDict &&) = default;
    JsonDict &operator=(JsonDict &&) = default;
    JsonDict(const JsonDict &) = default;
    JsonDict &operator=(const JsonDict &) = default;
    explicit JsonDict(const QVariant& src, bool nest = true, QChar separator = ':');
    explicit JsonDict(const QVariant& src, const QString &separator);
    JsonDict(const QVariantMap& src, bool nest = true, QChar separator = ':');
    JsonDict(const QVariantMap& src, const QString &separator);
    JsonDict(std::initializer_list<std::pair<QString, QVariant>> initializer);
    explicit JsonDict(QVariantMap&& src, bool nest = true, const QString &separator = ":");
    //! \warning Implicitly covertible to QVariant and QVariantMap
    operator const QVariantMap&() const&;
    operator QVariantMap&() &;
    operator QVariantMap&&() &&;
    QVariant toVariant() const;
    struct iterator;
    struct const_iterator;
    //! Функция доступа к вложенным элементам.
    /// \warning Попытка доступа к несуществующему ключу создает пустое значение в нем,
    /// не повлияет на данные, но стоит быть внимательным (возвращает QVariant& доступный для модификации)
    QVariant& operator[](const iterator& akey);
    QVariant& operator[](const const_iterator& akey);
    QVariant& operator[](const QStringList& akey);
    QVariant& operator[](const QString& akey);
    void insert(const QStringList& akey, const JsonDict &value);
    void insert(const QStringList& akey, const QVariant &value);
    void insert(const QStringList& akey, const QVariantMap &value);
    void insert(const QString& akey, const QVariantMap &value, QChar sep = ':');
    //! Move optimisations
    void insert(const QStringList& akey, JsonDict &&value);
    void insert(const QStringList& akey, QVariant &&value);
    void insert(const QStringList& akey, QVariantMap &&value);
    void insert(const QString& akey, const QVariant &value, QChar sep = ':');
    void insert(const QString& akey, QVariant &&value, QChar sep = ':');
    void insert(const QString& akey, QVariantMap &&value, QChar sep = ':');
    size_t depth() const;
    void swap(QVariantMap &dict) noexcept;
    const QVariant operator[](const QStringList& akey) const;
    const QVariant operator[](const QString& akey) const;
    //! Не создает веток по несуществующим ключам
    bool isValid(const QStringList& akey) const;
    bool isValid(const QString& akey) const;
    QList<QVariant> values() const;
    const QVariant value(const QString& akey, const QVariant &adefault = QVariant(), QChar sep = ':') const;
    const QVariant value(const QStringList& akey, const QVariant &adefault = QVariant()) const;
    //! Оператор глубокого сравнения словарей
    bool operator==(const JsonDict& src) const;
    bool operator!=(const JsonDict& src) const;
    void clear();
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
    QVariant take(const QString &akey, QChar separator = ':');
    QVariant take(const QString &akey, const QString &separator);
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

struct JsonDict::const_iterator {
    using iter = QVariantMap::const_iterator;
    ~const_iterator();
    const_iterator(iter begin, iter end, const void *container);
    const_iterator(const const_iterator &other);
    const_iterator(const_iterator &&other);
    const_iterator &operator=(const const_iterator &other);
    const_iterator &operator=(const_iterator &&other);
    bool operator==(const const_iterator &other) const;
    bool operator!=(const const_iterator &other) const;
    QStringList key() const;
    QStringList domainKey() const;
    const QVariantMap *domainMap() const;
    const QVariantList *domainList() const;
    QString field() const;
    int depth() const;
    bool isDomainMap() const;
    bool isDomainList() const;
    const QVariant &value() const;
    template <typename T>
    const T value() const {
        return value().template value<T>();
    }
    const_iterator &operator++();
    const_iterator operator++(int);
    const_iterator &operator*();
    const QVariant *operator->();
    template<std::size_t Index>
    std::tuple_element_t<Index, ::JsonDict::const_iterator>& get()
    {
        if constexpr (Index == 0) return key();
        if constexpr (Index == 1) return value();
    }
private:
    struct Private;
    Private *d;
};

struct JsonDict::iterator {
    using iter = QVariantMap::iterator;
    ~iterator();
    iterator(iter begin, iter end, void *container);
    iterator(const iterator &other);
    iterator(iterator &&other);
    iterator &operator=(const iterator &other);
    iterator &operator=(iterator &&other);
    bool operator==(const iterator &other) const;
    bool operator!=(const iterator &other) const;
    QStringList key() const;
    QStringList domainKey() const;
    const QVariantMap *domainMap() const;
    const QVariantList *domainList() const;
    QString field() const;
    int depth() const;
    bool isDomainMap() const;
    bool isDomainList() const;
    QVariant &value() const;
    template <typename T>
    T value() const {
        return value().template value<T>();
    }
    iterator &operator++();
    iterator operator++(int);
    iterator &operator*();
    QVariant *operator->();
    template<std::size_t Index>
    std::tuple_element_t<Index, ::JsonDict::iterator>& get()
    {
        if constexpr (Index == 0) return key();
        if constexpr (Index == 1) return value();
    }
private:
    struct Private;
    Private *d;
};

namespace std
{
template<>
struct tuple_size<::JsonDict::iterator>
{
    static constexpr size_t value = 2;
};
template<>
struct tuple_element<0, ::JsonDict::iterator>
{
    using type = QStringList;
};

template<>
struct tuple_element<1, ::JsonDict::iterator>
{
    using type = QVariant&;
};

template<std::size_t Index>
std::tuple_element_t<Index, ::JsonDict::iterator>& get(::JsonDict::iterator &iter)
{
    if constexpr (Index == 0) return iter.key();
    if constexpr (Index == 1) return iter.value();
}
template<>
struct tuple_size<::JsonDict::const_iterator>
{
    static constexpr size_t value = 2;
};
template<>
struct tuple_element<0, ::JsonDict::const_iterator>
{
    using type = QStringList;
};

template<>
struct tuple_element<1, ::JsonDict::const_iterator>
{
    using type = const QVariant&;
};

}
namespace Radapter {
    namespace literals {
        JsonDict operator "" _json(const char* str, std::size_t n);
    }
}
QDebug operator<<(QDebug dbg, const JsonDict &json);

/*! @} */ //JsonDict doxy
Q_DECLARE_METATYPE(JsonDict)
Q_DECLARE_TYPEINFO(JsonDict, Q_MOVABLE_TYPE);

#endif // JsonDict_H
