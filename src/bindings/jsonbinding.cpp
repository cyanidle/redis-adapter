#include "jsonbinding.h"
#include "bindingsprovider.h"
#include "private/brokerlogging.h"

JsonBinding::JsonBinding(const QString &name, const JsonDict &json) :
    m_name(name),
    m_mappings(),
    m_requiredKeys(),
    m_valueNames()
{
    if (json.isEmpty()) {
        throw std::runtime_error(std::string("Empty binding data for bind: ") + name.toStdString());
    }
    for (const auto &iter: json) {
        auto currentMapping = Mapping();
        if (iter.depth() > 1) throw std::invalid_argument("Incorrect Syntax! Flat Dictionaries only!");
        if (!iter.value().isValid()) throw std::invalid_argument("Invalid Key!");
        if (!iter.value().canConvert<QString>()) throw std::invalid_argument("Non String key!");
        if (iter.value().toString() == ignoreField) {
            m_ignored.append(iter.field());
            continue;
        }
        currentMapping.valueName = iter.field();
        m_valueNames.append(currentMapping.valueName);
        currentMapping.mappedKey = iter.value().toString().replace(".", ":");
        for (const auto &key : currentMapping.mappedKey.split(":")) {
            if (key.contains(matcher())) {
                auto newReq = key.mid(1,key.length()-2);
                if (!m_requiredKeys.contains(newReq)) m_requiredKeys.append(newReq);
            }
        }
        m_mappings.append(currentMapping);
    }
}

JsonBinding JsonBinding::optimise(const KeysFilter &keysFilter) const
{
    if (m_isOptimised) throw std::runtime_error("Attempt to optimised already optimised Binding!");
    JsonBinding result(*this);
    result.m_isOptimised = true;
    checkKeys(keysFilter, "optimise()");
    for (auto &mapping : result.m_mappings) {
        for (auto iter = keysFilter.constBegin(); iter != keysFilter.constEnd(); ++iter) {
            mapping.optimisedKey = mapping.mappedKey.replace(QStringLiteral("{%1}").arg(iter.key()), iter.value()).split(":");
        }
        mapping.mappedKey.clear();
    }
    return result;
}

void JsonBinding::checkKeys(const KeysFilter &keysFilter, const QString &funcName) const
{
    auto keys = keysFilter.keys();
    if (!unorderedEqual(keys, m_requiredKeys)) {
        throw std::runtime_error(funcName.toStdString() + ": Binding error (Keys mismatch)!" +
                                 "\n Name: (" +
                                 m_name.toStdString() + ")" +
                                 "\n Has Keys: " +
                                 m_requiredKeys.join(" | ").toStdString() +
                                 "\n Received: " + keys.join(" | ").toStdString());
    }
}

JsonBinding::Map JsonBinding::parseMap(const QVariantMap &src)
{
    auto result = Map();
    for (auto iter = src.begin(); iter != src.end(); ++iter) {
        auto currentName = iter.key();
        auto currentJson = iter.value().toMap();
        result.insert(currentName, JsonBinding(currentName, currentJson));
    }
    return result;
}

JsonBinding::JsonBinding() :
    m_name("Error")
{

}

JsonBinding::Result JsonBinding::receive(const JsonDict &msg, const KeysFilter &keysFilter) const
{
    if (!m_isOptimised) checkKeys(keysFilter, Q_FUNC_INFO);
    else if (!keysFilter.isEmpty()) throw std::runtime_error("Keys Passed to optimised binding!");
    auto result = JsonDict();
    quint16 mappingsWithoutValueName = 0;
    for (auto &mapping : m_mappings) {
        if (mapping.valueName.isEmpty()) {
            if (++mappingsWithoutValueName >= m_mappings.size()) {
                bindingsError() << "Binding (" << m_name <<
                    "): receive(): Could not receive msg, not a single named value!";
                throw std::runtime_error("Binding receive() error!");
            }
            continue;
        }
        if (m_isOptimised) {
            if (msg.contains(mapping.optimisedKey)) {
                result[mapping.valueName] = msg[mapping.optimisedKey];
            }
        } else {
            auto realKey = mapping.mappedKey;
            for (auto iter = keysFilter.constBegin(); iter != keysFilter.constEnd(); ++iter) {
                realKey.replace(QStringLiteral("{%1}").arg(iter.key()), iter.value());
            }
            auto splitKey = realKey.split(":");
            if (msg.contains(splitKey)) {
                result[mapping.valueName] = msg[splitKey];
            }
        }
    }
    return {std::move(result), availableValueNames()};
}

JsonDict JsonBinding::send(const JsonDict &values, const KeysFilter &keysFilter) const
{
    if (!m_isOptimised) {
        checkKeys(keysFilter, Q_FUNC_INFO);
    }
    else if (!keysFilter.isEmpty()) throw std::runtime_error("Keys Passed to optimised binding!");
    auto result = JsonDict();
    for (auto &mapping : m_mappings) {
        if (!values.contains(mapping.valueName)) continue;
        if (m_isOptimised) {
            result[mapping.optimisedKey] = values[mapping.valueName];
        } else {
            auto realKey = mapping.mappedKey;
            for (auto iter = keysFilter.constBegin(); iter != keysFilter.constEnd(); ++iter) {
                realKey.replace(QStringLiteral("{%1}").arg(iter.key()), iter.value());
            }
            result[realKey.split(":")] = values[mapping.valueName];
        }
    }
    return result;
}

const QRegExp &JsonBinding::matcher()
{
    static QRegExp result("^\\{\\w*\\}$");
    return result;
}

void JsonBinding::checkValues(const JsonDict &values, const QString &funcName) const
{
    auto keys = values.keysDeep();
    if (!unorderedEqual(keys, availableValueNames())) {
        bindingsError() << "Binding (" << m_name << "):" << funcName << ": Error:";
        bindingsError() << "Filter did not provide enough value names or provided excess ones!";
        bindingsError() << "Needed: " << availableValueNames() << "; Filter has: " << keys;
        throw std::runtime_error("Binding error!");
    }
}

const QVariant JsonBinding::Result::value(const QString &key) const
{
    if (!m_availableFields.contains(key)) {
        throw std::runtime_error(std::string("Unavailable key requested: ") + key.toStdString());
    }
    return m_dict[key];
}

QVariant &JsonBinding::Result::value(const QString &key)
{
    if (!m_availableFields.contains(key)) {
        throw std::runtime_error(std::string("Unavailable key requested: ") + key.toStdString());
    }
    return m_dict[key];
}

