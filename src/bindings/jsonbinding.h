#ifndef JSONBINDING_H
#define JSONBINDING_H

#include "private/global.h"
#include "jsondict/jsondict.hpp"
#include <QVariantMap>

class RADAPTER_SHARED_SRC JsonBinding
{
public:
    static constexpr const char *ignoreField = "<ignore>";
    struct Result;
    typedef QMap<QString, QString> KeysFilter;
    typedef QMap<QString, JsonBinding> Map;

    static Map parseMap(const QVariantMap &src);

    JsonBinding();
    JsonBinding(const QString &name, const JsonDict &json);
    JsonBinding optimise(const KeysFilter &keysFilter) const;
    Result receive(const JsonDict &msg, const KeysFilter &keysFilter = {}) const;
    JsonDict send(const JsonDict &values, const KeysFilter &keysFilter = {}) const;
    void requireValueName(const QString &valueName) const;
    const QString &name() const;
    const QStringList &ignoredFields() const {return m_ignored;}
    const QStringList &requiredKeys() const {return m_requiredKeys;}
    const QStringList &availableValueNames() const {return m_valueNames;}
    struct Result {
        friend class JsonBinding;
        const QVariant value(const QString &key) const;
        QVariant &value(const QString &key);
        const JsonDict &data() const {return m_dict;}
        const QStringList &available() const {return m_availableFields;}
        bool isEmpty() const {return m_dict.isEmpty();}
    private:
        Result() : m_dict(), m_availableFields() {}
        Result(JsonDict&& src, const QStringList &fields) : m_dict(std::move(src)), m_availableFields(fields) {}
        JsonDict m_dict;
        QStringList m_availableFields;
    };
private:
    static const QRegExp &matcher();
    void checkValues(const JsonDict &values, const QString &funcName) const;
    void checkKeys(const KeysFilter &keysFilter, const QString &funcName) const;
    template<typename T>
    static bool unorderedEqual(const QList<T> &target, const QList<T> &source);

    struct Mapping {
        QString mappedKey;
        QString valueName;
        QStringList optimisedKey;
    };
    QString m_name{};
    QList<Mapping> m_mappings{};
    QStringList m_requiredKeys{};
    QStringList m_valueNames{};
    QStringList m_ignored{};
    bool m_isOptimised{false};
    static QRegExp sm_matcher;
};

inline void JsonBinding::requireValueName(const QString &valueName) const
{
    if (!availableValueNames().contains(valueName)) {
        throw std::runtime_error("Binding: (" + name().toStdString() +
                                 ") does not provide: " +
                                 valueName.toStdString());
    }
}

inline const QString &JsonBinding::name() const {return m_name;}

Q_DECLARE_TYPEINFO(JsonBinding, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(JsonBinding::Result, Q_MOVABLE_TYPE);

template<typename T>
bool JsonBinding::unorderedEqual(const QList<T> &target, const QList<T> &source) {
    for (const auto &item : source) {
        if (!target.contains(item)) {
            return false;
        }
    }
    for (const auto &item : target) {
        if (!source.contains(item)) {
            return false;
        }
    }
    return true;
}

#endif // JSONBINDING_H
