#include "serializablesetting.h"
#include "jsondict/jsondict.h"
#include "templates/algorithms.hpp"
#include <QStringBuilder>

namespace Settings {

Serializable::Serializable() :
    m_allowExtra(false)
{
}

QString Serializable::print() const
{
    return QString(metaObject()->className())%' '%JsonDict{serialize()}.print();
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
    auto fieldRepr = found->fieldRepr(this);
    if (!newValue.isValid()) {
        if (hasDefault || isOptional) {
            return;
        } else {
            throw std::runtime_error(std::string(metaObject()->className())
                                     + ": Missing value for: "
                                     + name.toStdString()
                                     + "; Of Type: "
                                     + fieldRepr.toStdString());
        }
    }
    auto wasUpdated = found->updateWithVariant(this, newValue);
    if (wasUpdated) return;
    auto msg = QStringLiteral("Field '%1': Wanted: %2; Received: %3").arg(name, fieldRepr, newValue.typeName());
    throw std::runtime_error(std::string(metaObject()->className()) + ": Value type missmatch: " + msg.toStdString());
}

bool Serializable::update(const QVariantMap &src)
{
    try {
        allowExtra(m_allowExtra || src.value("__allow_extra__").toBool());
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

QString Serializable::getComment(const QString &fieldName) const
{
    auto ind = metaObject()->indexOfProperty(("__field_comment__" + fieldName).toStdString().c_str());
    return metaObject()->property(ind).readOnGadget(this).toString();
}

QString Serializable::getClassComment() const
{
    auto ind = metaObject()->indexOfProperty("__root_comment__");
    return metaObject()->property(ind).readOnGadget(this).toString();
}

FieldExample Serializable::getExample(const ::Serializable::FieldConcept *field) const
{
    if (field->isNested(this)) {
        return getExampleNested(field);
    } else {
        return getExamplePlain(field);
    }
}

FieldExample Serializable::getExamplePlain(const ::Serializable::FieldConcept *field) const
{
    static QList<QPair<QString, QString>> knownConversions{
       {"QString", "string"},
       {"QMap", "map"},
       {"QList", "list"},
       {"QStringList", "stringlist"},
       {"QVariantMap", "map<string, any>"},
       {"QVariantList", "list<any>"},
       {"QVariant", "any"},
    };
    FieldExample result;
    auto prepareTypeName = [&](QString &name) {
        for (auto iter = knownConversions.cbegin(); iter != knownConversions.cend(); ++iter) {
            name.replace(iter->first, iter->second);
        }
        return name;
    };
    auto typeName = field->typeName(this);
    result.typeName = prepareTypeName(typeName);
    result.defaultValue = field->readVariant(this);
    if (field->isSequence(this)) {
        result.fieldType = FieldExample::FieldSequence;
    } else if (field->isMapping(this)) {
        result.fieldType = FieldExample::FieldMapping;
    }
    return result;
}

FieldExample Serializable::getExampleNested(const ::Serializable::FieldConcept *field) const
{
    FieldExample result;
    auto getFromNested = [&](const ::Serializable::Object *asObj) {
        auto asSetting = asObj->as<Settings::Serializable>();
        if (!asSetting) {
            settingsParsingWarn()
                << "### Non Settings::Serializable used as FIELD()! Who:"
                << metaObject()->className()
                << "; Field:" << field->introspect(this).asObject()->metaObject()->className();
            return Example{};
        } else {
            return asSetting->getExample();
        }
    };
    if (field->isSequence(this)) {
        result.nestedType = FieldExample::NestedSequence;
        auto list = field->introspect(this).asObjectsList();
        QScopedPointer<Serializable::Object> ptr;
        if (list.isEmpty()) {
            ptr.reset(field->constructNested(this));
            list.append(ptr.data());
        }
        result.nested.reset(new Example(getFromNested(list.constFirst())));
    } else if (field->isMapping(this)) {
        result.nestedType = FieldExample::NestedMapping;
        auto map = field->introspect(this).asObjectsMap();
        QScopedPointer<Serializable::Object> ptr;
        if (map.isEmpty()) {
            ptr.reset(field->constructNested(this));
            map.insert("<name>", ptr.data());
        }
        result.nested.reset(new Example(getFromNested(map.first())));
    } else {
        result.nestedType = FieldExample::NestedField;
        result.nested.reset(new Example(getFromNested(field->introspect(this).asObject())));
    }
    return result;
}

Example Serializable::getExample() const
{
    Example result;
    for (auto &name: fields()) {
        auto current = getExample(field(name));
        current.comment = getComment(name);
        current.attributes = field(name)->attributes(this);
        result.fields.insert(name, current);
    }
    result.comment = getClassComment();
    return result;
}

void Serializable::allowExtra(bool state)
{
    m_allowExtra = state;
}

} // namespace Settings
