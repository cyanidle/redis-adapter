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

bool SerializableSettings::update(const QVariantMap &src)
{
    for (auto key = src.keyBegin(); key != src.keyEnd(); ++key) {
        if (!m_allowExtra && !fields().contains(*key)) {
            throw std::runtime_error("Extra field passed to Serializable Setting: "
                                     + std::string(metaObject()->className())
                                     + " --> " + key->toStdString());
        }
    }
    for (const auto &fieldName : fields()) {
        auto found = field(fieldName);
        auto valueFromSource = src.value(fieldName);
        auto required = !found->attributes(this).contains(NON_REQUIRED_ATTR);
        if (!valueFromSource.isValid() && !required) {
            continue;
        }
        auto wasUpdated = found->updateWithVariant(this, valueFromSource);
        if (!wasUpdated) {
            auto fieldTypeName = QStringLiteral("%1<%2>").arg(found->typeName(this), QString(QMetaType(found->valueMetaTypeId(this)).name()));
            if (src.contains(fieldName)) {
                auto received = QString(src[fieldName].typeName());
                if (received.isEmpty()) {
                    auto asJson = JsonDict(src[fieldName].toMap());
                    if (!asJson.isEmpty()) {
                        received = asJson.printDebug();
                    }
                }
                auto msg = QStringLiteral("Field '%1': Wanted: %2; Received: %3").arg(fieldName, fieldTypeName, received);
                throw std::runtime_error(std::string(metaObject()->className()) + ": Value type missmatch: " + msg.toStdString());
            } else {
                if (required) {
                    throw std::runtime_error(std::string(metaObject()->className())
                                             + ": Missing value for: "
                                             + fieldName.toStdString()
                                             + "; Of Type: "
                                             + fieldTypeName.toStdString());
                } else {
                    settingsParsingWarn().nospace()
                        << metaObject()->className()
                        << ": Error in Field: " << fieldName
                        << "; Of Type: " << fieldTypeName;
                }
            }
        }
    }
    postUpdate();
    return true;
}

void SerializableSettings::allowExtra(bool state)
{
    m_allowExtra = state;
}

} // namespace Settings
