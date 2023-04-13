#include "jsondict.h"

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
    if (!val || val->typeId() != QMetaType::QVariantMap) return 0;
    return reinterpret_cast<QVariantMap*>(val->data())->remove(akey.constLast());
}

QVariant JsonDict::take(const QStringList &akey)
{
    auto val = recurseTo(akey, 1);
    if (!val || val->typeId() != QMetaType::QVariantMap) return {};
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
    return JsonDict(json.toVariantMap(), ':', false);
}

JsonDict JsonDict::fromJson(const QByteArray &json, QJsonParseError *err)
{
    return JsonDict(QJsonDocument::fromJson(json, err).toVariant().toMap());
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

QVariant JsonDict::take(const QString &akey)
{
    return m_dict.take(akey);
}

bool JsonDict::isEmpty() const
{
    return m_dict.isEmpty();
}

JsonDict JsonDict::diff(const JsonDict &other) const
{
    JsonDict result;
    for (auto &iter : other) {
        auto key = iter.key();
        if (!contains(key)) {
            result.insert(key, iter.value());
        }
    }
    for (auto &iter : *this) {
        auto key = iter.key();
        if (!other.contains(key)) {
            result.insert(key, iter.value());
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

void JsonDict::insert(const QString &akey, const QVariant &value)
{
    m_dict.insert(akey, value);
}

void JsonDict::insert(const QStringList &akey, QVariant &&value)
{
    (*this)[akey] = std::move(value);
}

void JsonDict::insert(const QString &akey, QVariant &&value)
{
    m_dict.insert(akey, std::move(value));
}

void JsonDict::insert(const QStringList &akey, const QVariantMap &value)
{
    (*this)[akey] = value;
}

void JsonDict::insert(const QString &akey, const QVariantMap &value)
{
    m_dict.insert(akey, value);
}

void JsonDict::insert(const QStringList &akey, QVariantMap &&value)
{
    (*this)[akey] = std::move(value);
}

void JsonDict::insert(const QString &akey, QVariantMap &&value)
{
    m_dict.insert(akey, std::move(value));
}

void JsonDict::insert(const QStringList &akey, JsonDict &&value)
{
    (*this)[akey] = static_cast<QVariantMap&&>(std::move(value));
}

const QVariant JsonDict::operator[](const QString &akey) const
{
    return m_dict.value(akey);
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

const QVariant JsonDict::value(const QString &akey, const QVariant &adefault) const
{
    return m_dict.value(akey, adefault);
}

JsonDict::JsonDict(const QVariant &src, const QString &separator, bool nest) :
    m_dict(src.toMap())
{
    if (!isEmpty() && nest) this->nest(separator);
}

JsonDict::JsonDict(const QVariantMap &src, QChar separator, bool nest) :
    m_dict(src)
{
    if (!isEmpty() && nest) this->nest(separator);
}

JsonDict::JsonDict(const QVariant &src, QChar separator, bool nest) :
    m_dict(src.toMap())
{
    if (!isEmpty() && nest) this->nest(separator);
}

JsonDict::JsonDict(const QVariantMap &src, const QString &separator, bool nest) :
    m_dict(src)
{
    if (!isEmpty() && nest) this->nest(separator);
}

JsonDict::JsonDict(std::initializer_list<std::pair<QString, QVariant> > initializer) :
    m_dict(initializer)
{
    this->nest(':');
}

JsonDict::JsonDict(QVariantMap &&src, const QString &separator, bool nest) :
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

void JsonDict::swap(QVariantMap &dict) noexcept
{
    m_dict.swap(dict);
}

QString JsonDict::printDebug() const
{
    auto result = QStringLiteral("Json {");
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
    dbg.noquote() << json.printDebug();
    return dbg;
}
