#include "serializablesettings.h"
#include "jsondict/jsondict.h"
#include "templates/algorithms.hpp"

namespace Settings {

SerializableSettings::SerializableSettings() :
    m_allowExtra(false)
{
}

QString SerializableSettings::print() const
{
    return JsonDict{serialize()}.printDebug().replace("Json", metaObject()->className());
}

void SerializableSettings::checkForExtra(const QVariantMap &src)
{
    for (auto key = src.keyBegin(); key != src.keyEnd(); ++key) {
        if (!m_allowExtra && !fields().contains(*key)) {
            throw std::runtime_error("Extra field passed to Serializable Setting: "
                                     + std::string(metaObject()->className())
                                     + " --> " + key->toStdString());
        }
    }
}

void SerializableSettings::processField(const QString &name, const QVariant &newValue)
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

bool SerializableSettings::update(const QVariantMap &src)
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
                                 + allFields.toStdString() + ")");
    }
    postUpdate();
    return true;
}

void SerializableSettings::allowExtra(bool state)
{
    m_allowExtra = state;
}

} // namespace Settings
