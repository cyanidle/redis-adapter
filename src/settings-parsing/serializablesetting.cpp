#include "serializablesetting.h"
#include "jsondict/jsondict.h"
#include "templates/algorithms.hpp"

namespace Settings {

Serializable::Serializable() :
    m_allowExtra(false)
{
}

QString Serializable::print() const
{
    return JsonDict{serialize()}.printDebug().replace("Json", metaObject()->className());
}

void Serializable::checkForExtra(const QVariantMap &src)
{
    for (auto key = src.keyBegin(); key != src.keyEnd(); ++key) {
        if (!m_allowExtra && !fields().contains(*key)) {
            throw std::runtime_error("Extra field passed to Serializable Setting: "
                                     + std::string(metaObject()->className())
                                     + " --> " + key->toStdString());
        }
    }
}

void Serializable::processField(const QString &name, const QVariant &newValue)
{
    m_currentField = name;
    auto found = field(name);
    auto hasDefault = found->attributes(this).contains(HAS_DEFAULT_ATTR);
    auto isOptional = found->attributes(this).contains(OPTION_ATTR);
    auto fieldTypeName = found->fieldRepr(this);
    if (!newValue.isValid()) {
        if (hasDefault || isOptional) {
            return;
        } else {
            throw std::runtime_error(std::string(metaObject()->className())
                                     + ": Missing value for: "
                                     + name.toStdString()
                                     + "; Of Type: "
                                     + fieldTypeName.toStdString());
        }
    }
    auto wasUpdated = found->updateWithVariant(this, newValue);
    if (wasUpdated) return;
    auto msg = QStringLiteral("Field '%1': Wanted: %2; Received: %3").arg(name, fieldTypeName, newValue.typeName());
    throw std::runtime_error(std::string(metaObject()->className()) + ": Value type missmatch: " + msg.toStdString());
}

bool Serializable::update(const QVariantMap &src)
{
    try {
        if (!m_allowExtra) {
            checkForExtra(src);
        }
        for (const auto &fieldName : fields()) {
            processField(fieldName, src.value(fieldName));
        }
    } catch (std::runtime_error &err) {
        auto allFields = fields().join(", ");
        if (!m_currentField.isEmpty()) {
            allFields.replace(m_currentField, "--> " + m_currentField + " <--");
        }
        for (auto &name: fields()) {
            if (field(name)->isSequence(this)) {
                allFields.replace(name, name + "[]");
            }
            if (field(name)->isMapping(this)) {
                allFields.replace(name, name + "{}");
            }
        }
        throw std::runtime_error(err.what()
                                 + std::string("\n# in ") + metaObject()->className()
                                 + std::string(" (")
                                 + allFields.toStdString() + ')');
    }
    postUpdate();
    return true;
}

QVariant Serializable::printExample(const ::Serializable::FieldConcept *field) const
{
    if (field->isNested(this)) {
        return printExampleNested(field);
    } else {
        return printExamplePlain(field);
    }
}

QVariant Serializable::printExamplePlain(const ::Serializable::FieldConcept *field) const
{
    static QMap<QString, QString> knownConversions{
       {"QString", "String"},
       {"QMap", "Map"},
       {"QList", "List"},
       {"QStringList", "StringList"},
       {"QVariantMap", "Map<String, Any>"},
       {"QVariantList", "List<Any>"},
       {"QVariant", "Any"},
    };
    auto prepareTypeName = [&](QString &name) {
        for (auto iter = knownConversions.cbegin(); iter != knownConversions.cend(); ++iter) {
            name.replace(iter.key(), iter.value());
        }
        return name;
    };
    if (field->isSequence(this)) {
        auto typeName = field->typeName(this);
        auto res = QStringLiteral("[<value>, <%1>...]").arg(prepareTypeName(typeName));
        QVariantList result{res};
        return result;
    } else if (field->isMapping(this)) {
        auto typeName = field->typeName(this);
        auto res = QStringLiteral("{<string>: <%1>...}").arg(prepareTypeName(typeName));
        QVariantMap result{{"<string>", res}};
        return result;
    } else {
        auto data = field->readVariant(this).toString();
        auto typeName = QStringLiteral("<%1>").arg(field->typeName(this));
        return prepareTypeName(typeName) + ' ' + '(' + data + ')';
    }
}

QVariant Serializable::printExampleNested(const ::Serializable::FieldConcept *field) const
{
    auto getFromNested = [&](const ::Serializable::Object *asObj) {
        auto asSetting = asObj->as<Settings::Serializable>();
        if (!asSetting) {
            settingsParsingWarn()
                << "### Non Settings::Serializable used as FIELD()! Who:"
                << metaObject()->className()
                << "; Field:" << field->introspect(this).asObject()->metaObject()->className();
            return QVariant{"Error"};
        } else {
            return QVariant(asSetting->printExample());
        }
    };
    if (field->isSequence(this)) {
        auto list = field->introspect(this).asObjectsList();
        QScopedPointer<Serializable::Object> ptr;
        if (list.isEmpty()) {
            ptr.reset(field->constructNested(this));
            list.append(ptr.data());
        }
        QVariantList result;
        for (auto &iter: list) {
            result.append(getFromNested(iter));
        }
        return result;
    } else if (field->isMapping(this)) {
        auto map = field->introspect(this).asObjectsMap();
        QScopedPointer<Serializable::Object> ptr;
        if (map.isEmpty()) {
            ptr.reset(field->constructNested(this));
            map.insert("<name>", ptr.data());
        }
        QVariantMap result;
        for (auto iter = map.cbegin(); iter != map.cend(); ++iter) {
            result.insert(iter.key(), getFromNested(iter.value()));
        }
        return result;
    } else {
        return getFromNested(field->introspect(this).asObject());
    }
}

QVariantMap Serializable::printExample() const
{
    QVariantMap result;
    for (auto &name: fields()) {
        result.insert(name, printExample(field(name)));
    }
    return result;
}

void Serializable::allowExtra(bool state)
{
    m_allowExtra = state;
}

} // namespace Settings
