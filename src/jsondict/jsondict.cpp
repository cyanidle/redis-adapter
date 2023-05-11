#include "jsondict.h"
#include "radapterlogging.h"

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

QVariantMap &JsonDict::top()
{
    return m_dict;
}

const QVariantMap &JsonDict::top() const
{
    return m_dict;
}

bool JsonDict::contains(const QString &key, QChar sep) const
{
    return contains(key.split(sep));
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
    for (const auto &otherItem: src)
    {
        auto thisItem = value(otherItem.key());
        if (!thisItem.isValid() || (thisItem != otherItem.value())) {
            return false;
        }
    }
    return true;

}

qint64 JsonDict::toIndex(const QString &key)
{
    bool isBounded = key.size() >= 3 && key.front() == '[' && key.back() == ']';
    bool ok;
    auto result = key.mid(1, key.length() - 2).toLong(&ok);
    return ok && isBounded ? result : -1;
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
        if (current->typeId() == QMetaType::QVariantMap) {
            recursingDict = true;
            currentDict = reinterpret_cast<const QVariantMap *>(current->data());
        } else if (current->typeId() == QMetaType::QVariantList) {
            recursingDict = false;
            currentList = reinterpret_cast<const QVariantList *>(current->data());
        } else if (i < fullKey.size() - ignoreLastKeys - 1){
            return nullptr;
        }
    }
    return current;
}

QVariant *JsonDict::recurseTo(const QStringList &fullKey, int ignoreLastKeys)
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
        if (current->typeId() == QMetaType::QVariantMap) {
            recursingDict = true;
            currentDict = reinterpret_cast<QVariantMap *>(current->data());
        } else if (current->typeId() == QMetaType::QVariantList) {
            recursingDict = false;
            currentList = reinterpret_cast<QVariantList *>(current->data());
        } else if (i < fullKey.size() - ignoreLastKeys - 1){
            return nullptr;
        }
    }
    return current;
}

const QVariant *JsonDict::find(const QVariantMap *dict, const QString &key)
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

QVariant *JsonDict::find(QVariantMap *dict, const QString &key)
{
    if (dict->contains(key)) {
        return &(*dict)[key];
    }
    return nullptr;
}

const QVariant *JsonDict::find(const QVariantList *list, qint64 index)
{
    if (index < 0) return nullptr;
    if (list->size() > index) {
        return &list->at(index);
    }
    return nullptr;
}

QVariant *JsonDict::find(QVariantList *list, qint64 index)
{
    if (index < 0) return nullptr;
    if (list->size() > index) {
        return &list->operator[](index);
    }
    return nullptr;
}

JsonDict::iterator JsonDict::begin()
{
    return JsonDict::iterator(m_dict.begin(), m_dict.end(), &m_dict);
}

JsonDict::iterator JsonDict::end()
{
    return JsonDict::iterator(m_dict.end(), m_dict.end(), &m_dict);
}

JsonDict::const_iterator JsonDict::begin() const
{
    return JsonDict::const_iterator(m_dict.begin(), m_dict.end(), &m_dict);
}

JsonDict::const_iterator JsonDict::end() const
{
    return JsonDict::const_iterator(m_dict.end(), m_dict.end(), &m_dict);
}

JsonDict::const_iterator JsonDict::cbegin() const
{
    return begin();
}

JsonDict::const_iterator JsonDict::cend() const
{
    return end();
}

const QVariant JsonDict::operator[](const QStringList &akey) const
{
    return value(akey);
}

void JsonDict::insert(const QStringList &akey, const JsonDict &value)
{
    (*this)[akey] = static_cast<const QVariantMap&>(value);
}

const QVariant JsonDict::value(const QStringList &akey, const QVariant &adefault) const
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
    if (val && val->typeId() != QMetaType::QVariantMap) {
        return reinterpret_cast<QVariantMap*>(val->data())->remove(akey.constLast());
    } else if (val && val->typeId() != QMetaType::QVariantList) {
        auto ind = toIndex(akey.constLast());
        if (ind == -1) return {};
        auto asList = reinterpret_cast<QVariantList*>(val->data());
        if (asList->size() <= ind) return {};
        asList->removeAt(ind);
        return 1;
    } else {
        return {};
    }
}

QVariant JsonDict::take(const QStringList &akey)
{
    auto val = recurseTo(akey, 1);
    if (val && val->typeId() != QMetaType::QVariantMap) {
        return reinterpret_cast<QVariantMap*>(val->data())->take(akey.constLast());
    } else if (val && val->typeId() != QMetaType::QVariantList) {
        auto ind = toIndex(akey.constLast());
        if (ind == -1) return {};
        auto asList = reinterpret_cast<QVariantList*>(val->data());
        if (asList->size() <= ind) return {};
        return asList->takeAt(ind);
    } else {
        return {};
    }
}

QVariant JsonDict::take(const QString &akey, const QString &separator)
{
    return take(akey.split(separator));
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

JsonDict &JsonDict::nest(const QString &separator)
{
    JsonDict newState;
    for (auto iter = m_dict.constBegin(); iter != m_dict.constEnd(); ++iter) {
        const auto &val = iter.value();
        QVariant toInsert;
        if (val.typeId() == QMetaType::QVariantList) {
            auto actual = QVariantList{};
            for (const auto &subval : *reinterpret_cast<const QVariantList*>(val.data())) {
                if (subval.typeId() == QMetaType::QVariantMap) {
                    actual.append(JsonDict(subval, separator).toVariant());
                } else {
                    actual.append(subval);
                }
            }
            toInsert = actual;
        } else if (val.typeId() == QMetaType::QVariantMap) {
            toInsert = JsonDict(val, separator).toVariant();
        } else {
            toInsert = val;
        }
        newState.insert(iter.key().split(separator), toInsert);
    }
    swap(newState);
    return *this;
}

JsonDict &JsonDict::merge(const JsonDict &src, bool overwrite)
{
    for (auto &[key, val] : src) {
        const auto &thisVal = value(key);
        if (overwrite || !thisVal.isValid()) {
            if (thisVal == val) continue;
            insert(key, val);
        }
    }
    return *this;
}

JsonDict JsonDict::nest(const QString &separator) const
{
    JsonDict result = *this;
    return result.nest(separator);
}

JsonDict &JsonDict::operator+=(const JsonDict &src)
{
    return merge(src);
}

JsonDict JsonDict::operator+(const JsonDict &src) const
{
    return merge(src);
}

JsonDict JsonDict::operator-(const JsonDict &src) const
{
    return diff(src);
}

JsonDict &JsonDict::operator-=(const JsonDict &src)
{
    return (*this) = diff(src);
}

bool JsonDict::update(const JsonDict &src, bool overwrite)
{
    bool wasUpdated = false;
    for (auto &[key, val] : src) {
        const auto &thisVal = value(key);
        if (overwrite || !thisVal.isValid()) {
            if (thisVal == val) continue;
            wasUpdated = true;
            insert(key, val);
        }
    }
    return wasUpdated;
}

JsonDict JsonDict::merge(const JsonDict &src) const
{
    JsonDict result;
    return result.merge(src, true);
}

QVariant &JsonDict::operator[](const iterator &akey)
{
    return (*this)[akey.key()];
}

QVariant &JsonDict::operator[](const const_iterator &akey)
{
    return (*this)[akey.key()];
}

QVariant &JsonDict::operator[](const QStringList &akey)
{
    if (akey.isEmpty()) {
        throw std::invalid_argument("Cannot access JsonDict with empty QStringList as key!");
    }
    QVariant* currentVal = &m_dict[akey.constFirst()];
    for (int index = 1; index < akey.size(); ++index) {
        auto &nextKey = akey[index];
        auto nextIndex = toIndex(nextKey);
        auto indexValid = nextIndex != -1;
        if (currentVal->isValid()) {
            if (currentVal->typeId() == QMetaType::QVariantMap) {
                if (indexValid) {
                    throw std::invalid_argument("Access to nested Dict as to List");
                }
                auto asDict = reinterpret_cast<QVariantMap*>(currentVal->data());
                currentVal = &(asDict->operator[](nextKey));
            } else if (currentVal->typeId() == QMetaType::QVariantList) {
                if (!indexValid) {
                    throw std::invalid_argument("Access to nested List as to Dict");
                }
                auto asList = reinterpret_cast<QVariantList*>(currentVal->data());
                asList->reserve(nextIndex + 1);
                while (asList->size() <= nextIndex) {
                    asList->append(QVariant{});
                }
                currentVal = &(asList->operator[](nextIndex));
            } else if (indexValid) {
                currentVal->setValue(QVariantList{});
                auto asList = reinterpret_cast<QVariantList*>(currentVal->data());
                while (asList->size() <= nextIndex) {
                    asList->append(QVariant{});
                }
                currentVal = &(asList->operator[](nextIndex));
            } else {
                currentVal->setValue(QVariantMap{});
                auto asDict = reinterpret_cast<QVariantMap*>(currentVal->data());
                currentVal = &(asDict->operator[](nextKey));
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

QByteArray JsonDict::toBytes(QJsonDocument::JsonFormat format) const
{
    return QJsonDocument(toJsonObj()).toJson(format);
}

JsonDict JsonDict::fromJsonObj(const QJsonObject &json)
{
    return JsonDict(json.toVariantMap(), false);
}

JsonDict JsonDict::fromBytes(const QByteArray &json, QJsonParseError *err)
{
    return JsonDict(QJsonDocument::fromJson(json, err).toVariant().toMap(), false);
}

QString JsonDict::processWarn(const QStringList &src, const int &index)
{
    QString result;
    result.append(src.constFirst());
    for (int i = 1; i < index;++i){
        result.append(":");
        result.append(src.at(i));
    }
    return result;
}

bool JsonDict::operator==(const JsonDict &src) const
{
    return m_dict == src.m_dict;
}

bool JsonDict::operator!=(const JsonDict &src) const
{
    return m_dict != src.m_dict;
}

void JsonDict::clear()
{
    m_dict.clear();
}

QVariant JsonDict::take(const QString &akey, QChar separator)
{
    return take(akey.split(separator));
}

bool JsonDict::isEmpty() const
{
    return !deepCount();
}

JsonDict JsonDict::diff(const JsonDict &other, bool full) const
{
    JsonDict result;
    if (full) {
        for (auto &[key, otherVal] : other) {
            const auto &thisVal = value(key);
            if (thisVal != otherVal) {
                result.insert(key, otherVal);
            }
        }
    }
    for (auto &[key, thisVal] : *this) {
        const auto &otherVal = other.value(key);
        if (otherVal != thisVal) {
            result.insert(key, thisVal);
        }
    }
    return result;
}

JsonDict &JsonDict::nest(QChar separator)
{
    JsonDict newState;
    for (auto iter = m_dict.constBegin(); iter != m_dict.constEnd(); ++iter) {
        const auto &val = iter.value();
        QVariant toInsert;
        if (val.typeId() == QMetaType::QVariantList) {
            auto actual = QVariantList{};
            for (const auto &subval : *reinterpret_cast<const QVariantList*>(val.data())) {
                if (subval.typeId() == QMetaType::QVariantMap) {
                    actual.append(JsonDict(subval, separator).toVariant());
                } else {
                    actual.append(subval);
                }
            }
            toInsert = actual;
        } else if (val.typeId() == QMetaType::QVariantMap) {
            toInsert = JsonDict(val, separator).toVariant();
        } else {
            toInsert = val;
        }
        newState.insert(iter.key().split(separator), toInsert);
    }
    swap(newState);
    return *this;
}

QStringList JsonDict::firstKey() const
{
    for (auto &iter : *this) {
        return iter.key();
    }
    throw std::invalid_argument("firstKey() on Empty JsonDict!");
}

QVariant &JsonDict::first()
{
    for (auto &iter : *this) {
        return iter.value();
    }
    throw std::invalid_argument("first() on Empty JsonDict!");
}

const QVariant &JsonDict::first() const
{
    for (auto &iter : *this) {
        return iter.value();
    }
    throw std::invalid_argument("first() on Empty JsonDict!");
}

QVariant &JsonDict::operator[](const QString &akey)
{
    return (*this)[akey.split(':')];
}

void JsonDict::insert(const QStringList &akey, const QVariant &value)
{
    (*this)[akey] = value;
}

void JsonDict::insert(const QString &akey, const QVariant &value, QChar sep)
{
    insert(akey.split(sep), value);
}

void JsonDict::insert(const QStringList &akey, QVariant &&value)
{
    (*this)[akey] = std::move(value);
}

void JsonDict::insert(const QString &akey, QVariant &&value, QChar sep)
{
    insert(akey.split(sep), std::move(value));
}

void JsonDict::insert(const QStringList &akey, const QVariantMap &value)
{
    (*this)[akey] = value;
}

void JsonDict::insert(const QString &akey, const QVariantMap &value, QChar sep)
{
    insert(akey.split(sep), value);
}

void JsonDict::insert(const QStringList &akey, QVariantMap &&value)
{
    (*this)[akey] = std::move(value);
}

void JsonDict::insert(const QString &akey, QVariantMap &&value, QChar sep)
{
    insert(akey.split(sep), std::move(value));
}

void JsonDict::insert(const QStringList &akey, JsonDict &&value)
{
    (*this)[akey] = static_cast<QVariantMap&&>(std::move(value));
}

const QVariant JsonDict::operator[](const QString &akey) const
{
    return value(akey);
}

bool JsonDict::isValid(const QStringList &akey) const
{
    return value(akey).isValid();
}

bool JsonDict::isValid(const QString &akey) const
{
    return m_dict.value(akey).isValid();
}

QList<QVariant> JsonDict::values() const
{
    QList<QVariant> result;
    for (auto &iter : *this) {
        result.append(iter.value());
    }
    return result;
}

const QVariant JsonDict::value(const QString &akey, const QVariant &adefault, const QString &sep) const
{
    return value(akey.split(sep), adefault);
}

const QVariant JsonDict::value(const QString &akey, const QVariant &adefault, QChar sep) const
{
    return value(akey.split(sep), adefault);
}

JsonDict::JsonDict(const QVariant &src, const QString &separator) :
    m_dict(src.toMap())
{
    if (!isEmpty()) this->nest(separator);
    if (src.isValid() && m_dict.isEmpty()) {
        reError() << "JsonDict received non-null non-map QVariant!";
    }
}

JsonDict::JsonDict(const QVariantMap &src, bool nest, QChar separator) :
    m_dict(src)
{
    if (!isEmpty() && nest) this->nest(separator);
}

JsonDict::JsonDict(const QVariant &src, bool nest, QChar separator) :
    m_dict(src.toMap())
{
    if (!isEmpty() && nest) this->nest(separator);
    if (src.isValid() && m_dict.isEmpty()) {
        reError() << "JsonDict received non-null non-map QVariant!";
    }
}

JsonDict::JsonDict(const QVariantMap &src, const QString &separator) :
    m_dict(src)
{
    if (!isEmpty()) this->nest(separator);
}

JsonDict::JsonDict(std::initializer_list<std::pair<QString, QVariant> > initializer, bool nest, QChar separator) :
    m_dict(initializer)
{
    if (!isEmpty() && nest) this->nest(separator);
}

JsonDict::JsonDict(QVariantMap &&src, bool nest, const QString &separator) :
    m_dict(std::move(src))
{
    if (!isEmpty() && nest) this->nest(separator);
}

JsonDict::operator const QVariantMap &() const &
{
    return m_dict;
}

JsonDict::operator QVariantMap &() &
{
    return m_dict;
}

JsonDict::operator QVariantMap &&() &&
{
    return std::move(m_dict);
}


QVariant JsonDict::toVariant() const
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

JsonDict JsonDict::sanitized() const
{
    JsonDict result;
    for(auto &[k, v] :*this) {
        result[k] = v;
    }
    return result;
}

void JsonDict::swap(QVariantMap &dict) noexcept
{
    m_dict.swap(dict);
}

QString JsonDict::print() const
{
    auto result = QStringLiteral("{");
    const auto flat = flatten();
    for (auto iter{flat.begin()}; iter != flat.end(); ++iter) {
        result += QStringLiteral("\n\t%1: %2").arg(iter.key(), iter.value().toString());
    }
    result += "\n}";
    return result;
}

QDebug operator<<(QDebug dbg, const JsonDict &json)
{
    QDebugStateSaver saver(dbg);
    dbg.noquote() << json.print();
    return dbg;
}

JsonDict Radapter::literals::operator "" _json(const char *str, std::size_t n)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray(str, static_cast<int>(n)), &err);
    if (!doc.isObject() || err.error != QJsonParseError::NoError) {
        throw std::runtime_error("Json Parse Error!");
    }
    return JsonDict::fromJsonObj(doc.object());
}

//namespace { // similar to static func(). Allows compiler to optimize better
template <bool isConst>
struct NestedIter {
    using list_iter = std::conditional_t<isConst, QVariantList::const_iterator, QVariantList::iterator>;
    using map_iter = std::conditional_t<isConst, QVariantMap::const_iterator, QVariantMap::iterator>;
    using qual_value_t = std::conditional_t<isConst, const QVariant, QVariant>;
    NestedIter() : is_valid(false) {}
    NestedIter(list_iter list, const void* container) :
        is_map(false),
        list(list),
        _container(container)
    {}
    NestedIter(map_iter map, const void* container) :
        is_map(true),
        map(map),
        _container(container)
    {}
    NestedIter(const NestedIter& other) :
        is_map(other.is_map),
        count(other.count),
        _container(other._container)
    {
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
    qual_value_t &value() const {
        assert(is_valid);
        return is_map ? map.value() : *list;
    }
    NestedIter &operator=(const NestedIter &other) {
        is_valid = other.is_valid;
        is_map = other.is_map;
        count = other.count;
        _container = other._container;
        if (is_map) map = other.map;
        else list = other.list;
        return *this;
    }
    bool operator==(const NestedIter &other) const {
        assert(is_valid);
        if (is_map != other.is_map) return false;
        if (_container != other._container) return false;
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
    map_iter map;
    list_iter list;
    const void* _container{nullptr};
};

struct iterator_private_common {
    enum IteratorFlagValues {
        None = 0,
        IsEnd = 1 << 1,
        IsInRecursion = 1 << 2,
    };
    Q_DECLARE_FLAGS(IterFlags, IteratorFlagValues);
    IterFlags flags;
    constexpr bool isEnd () const noexcept {
        return flags.testFlag(IsEnd);
    }
    constexpr bool isRecursion() const noexcept {
        return flags.testFlag(IsInRecursion);
    }
    void stopRecurse() {
        flags.setFlag(IsInRecursion, false);
    }
    void startRecurse() {
        flags.setFlag(IsInRecursion);
    }
};
//} // namespace

struct JsonDict::const_iterator::Private : iterator_private_common {
    using MapT = const QVariantMap;
    using ListT = const QVariantList;
    using Iter = NestedIter<true>;
    Iter current;
    Iter end;
    struct TraverseState {
        Iter current;
        Iter end;
    };
    QStack<TraverseState> traverseHistory;
    bool historyEmpty() const noexcept {
        return traverseHistory.isEmpty();
    }
};

struct JsonDict::iterator::Private : iterator_private_common {
    using MapT = QVariantMap;
    using ListT = QVariantList;
    using Iter = NestedIter<false>;
    Iter current;
    Iter end;
    struct TraverseState {
        Iter current;
        Iter end;
    };
    QStack<TraverseState> traverseHistory;
    bool historyEmpty() const noexcept {
        return traverseHistory.isEmpty();
    }
};
//namespace {
template <typename Priv>
struct PrivateLogic
{
    PrivateLogic (Priv *d) : d(d) {}
    QStringList key() const
    {
        if (d->historyEmpty()) {
            return {d->current.key()};
        }
        QStringList result;
        for (auto &state : d->traverseHistory) {
            result.append(state.current.key());
        }
        result.append(d->current.key());
        return result;
    }
    QStringList domainKey() const {
        if (d->historyEmpty()) {
            return {d->current.key()};
        }
        QStringList result;
        for (auto &state : d->traverseHistory) {
            result.append(state.current.key());
        }
        return result;
    }
    const QVariantMap *domainMap() const
    {
        if (!d->historyEmpty() && isDomainMap()) {
            return reinterpret_cast<const QVariantMap*>(d->traverseHistory.last().current.value().data());
        } else {
            return nullptr;
        }
    }
    const QVariantList *domainList() const
    {
        if (!d->historyEmpty() && isDomainList()) {
            return reinterpret_cast<const QVariantList*>(d->traverseHistory.last().current.value().data());
        } else {
            return nullptr;
        }
    }
    int depth() const {
        return d->traverseHistory.size();
    }

    bool isDomainMap() const
    {
        return d->current.is_map;
    }

    bool isDomainList() const
    {
        return !isDomainMap();
    }
    Priv &operator++()
    {
        if (!d->isRecursion()){
            ++d->current;
        }
        if (d->current == d->end) {
            if (d->historyEmpty()) {
                d->stopRecurse();
                return *d;
            }
            auto popped = d->traverseHistory.pop();
            d->current = popped.current;
            d->end = popped.end;
            ++d->current;
            d->startRecurse();
            return ++*this;
        }
        auto *val = &d->current.value();
        if (val->typeId() == QMetaType::QVariantMap) {
            d->traverseHistory.push(typename Priv::TraverseState{d->current, d->end});
            auto *asDict = reinterpret_cast<typename Priv::MapT*>(val->data());
            d->current = {asDict->begin(), asDict};
            d->end = {asDict->end(), asDict};
            d->startRecurse();
            return ++*this;
        } else if (val->typeId() == QMetaType::QVariantList) {
            d->traverseHistory.push(typename Priv::TraverseState{d->current, d->end});
            auto *asList = reinterpret_cast<typename Priv::ListT*>(val->data());
            d->current = {asList->begin(), asList};
            d->end = {asList->end(), asList};
            d->startRecurse();
            return ++*this;
        } else if (!val->isValid()) {
            ++d->current;
            d->startRecurse();
            return ++*this;
        }
        d->stopRecurse();
        return *d;
    }

    bool operator==(const Priv &other) const {
        return d->current == other.current;
    }

    QString field() const {
        return d->current.key();
    }
    Priv *d;
};

template <typename Priv>
PrivateLogic<Priv> logic(Priv *d) {
    return PrivateLogic{d};
}

template <typename Priv>
const PrivateLogic<Priv> logic(const Priv *d) {
    return PrivateLogic<Priv>(const_cast<Priv*>(d));
}

//} // namespace

JsonDict::iterator::~iterator()
{
    delete d;
}

JsonDict::iterator::iterator(iter start, iter end, void *container) :
    d(new Private{
        {start == end ? iterator_private_common::IsEnd : iterator_private_common::None},
        {start, container},
        {end, container},
        {}
    })
{
    if (!d->isEnd()) {
        d->startRecurse();
        ++(*this);
    }
}

JsonDict::iterator::iterator(const iterator &other) :
    d(new Private(*other.d))
{

}

JsonDict::iterator::iterator(iterator &&other) :
    d(new Private(std::move(*other.d)))
{

}

JsonDict::iterator &JsonDict::iterator::operator=(const iterator &other)
{
    *d = *other.d;
    return *this;
}

bool JsonDict::iterator::operator==(const iterator &other) const
{
    return logic(d) == *other.d;
}

bool JsonDict::iterator::operator!=(const iterator &other) const
{
    return !(*this == other);
}

QStringList JsonDict::iterator::key() const
{
    return logic(d).key();
}

QStringList JsonDict::iterator::domainKey() const
{
    return logic(d).domainKey();
}

const QVariantMap *JsonDict::iterator::domainMap() const
{
    return logic(d).domainMap();
}

const QVariantList *JsonDict::iterator::domainList() const
{
    return logic(d).domainList();
}

QString JsonDict::iterator::field() const
{
    return logic(d).field();
}

int JsonDict::iterator::depth() const
{
    return logic(d).depth();
}

bool JsonDict::iterator::isDomainMap() const
{
    return logic(d).isDomainMap();
}

bool JsonDict::iterator::isDomainList() const
{
    return logic(d).isDomainList();
}

QVariant &JsonDict::iterator::value() const
{
    return d->current.value();
}

JsonDict::iterator &JsonDict::iterator::operator++()
{
    ++logic(d);
    return *this;
}

JsonDict::iterator &JsonDict::iterator::operator*()
{
    return *this;
}

QVariant *JsonDict::iterator::operator->()
{
    return &value();
}

JsonDict::iterator JsonDict::iterator::operator++(int)
{
    auto temp = *this;
    ++logic(d);
    return temp;
}

JsonDict::iterator &JsonDict::iterator::operator=(iterator &&other)
{
    *d = std::move(*other.d);
    return *this;
}

JsonDict::const_iterator::~const_iterator()
{
    delete d;
}

JsonDict::const_iterator::const_iterator(iter start, iter end, const void *container) :
    d(new Private{
        {start == end ? iterator_private_common::IsEnd : iterator_private_common::None},
        {start, container},
        {end, container},
        {}
    })
{
    if (!d->isEnd()) {
        d->startRecurse();
        ++(*this);
    }
}

JsonDict::const_iterator::const_iterator(const const_iterator &other) :
    d(new Private(*other.d))
{

}

JsonDict::const_iterator::const_iterator(const_iterator &&other) :
    d(new Private(std::move(*other.d)))
{

}

JsonDict::const_iterator &JsonDict::const_iterator::operator=(const const_iterator &other)
{
    *d = *other.d;
    return *this;
}

bool JsonDict::const_iterator::operator==(const const_iterator &other) const
{
    return logic(d) == *other.d;
}

bool JsonDict::const_iterator::operator!=(const const_iterator &other) const
{
    return !(*this == other);
}

QStringList JsonDict::const_iterator::key() const
{
    return logic(d).key();
}

QStringList JsonDict::const_iterator::domainKey() const
{
    return logic(d).domainKey();
}

const QVariantMap *JsonDict::const_iterator::domainMap() const
{
    return logic(d).domainMap();
}

const QVariantList *JsonDict::const_iterator::domainList() const
{
    return logic(d).domainList();
}

QString JsonDict::const_iterator::field() const
{
    return logic(d).field();
}

int JsonDict::const_iterator::depth() const
{
    return logic(d).depth();
}

bool JsonDict::const_iterator::isDomainMap() const
{
    return logic(d).isDomainMap();
}

bool JsonDict::const_iterator::isDomainList() const
{
    return logic(d).isDomainList();
}

const QVariant &JsonDict::const_iterator::value() const
{
    return d->current.value();
}

JsonDict::const_iterator &JsonDict::const_iterator::operator++()
{
    ++logic(d);
    return *this;
}

JsonDict::const_iterator &JsonDict::const_iterator::operator*()
{
    return *this;
}

const QVariant *JsonDict::const_iterator::operator->()
{
    return &value();
}

JsonDict::const_iterator JsonDict::const_iterator::operator++(int)
{
    auto temp = *this;
    ++logic(d);
    return temp;
}

JsonDict::const_iterator &JsonDict::const_iterator::operator=(const_iterator &&other)
{
    *d = std::move(*other.d);
    return *this;
}
