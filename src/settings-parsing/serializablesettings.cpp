#include "serializablesettings.h"
#include "jsondict/jsondict.hpp"
#include "templates/algorithms.hpp"

namespace Settings {

QString SerializableSettings::print() const
{
    return JsonDict{serialize()}.printDebug().replace("Json", metaObject()->className());
}

bool SerializableSettings::update(const QVariantMap &src)
{
    fillFields();
    for (const auto &fieldName : fields()) {
        auto found = field(fieldName);
        if (!found->updateWithVariant(src.value(fieldName)) && !found->attributes().contains("non_required")) {
            auto fieldTypeName = QStringLiteral("%1<%2>").arg(found->typeName(), QString(QMetaType(found->valueMetaTypeId()).name()));
            if (src.contains(fieldName)) {
                auto msg = QStringLiteral("Wanted: %1; Received: %2").arg(fieldTypeName, src[fieldName].toString());
                throw std::runtime_error("Value type missmatch: " + msg.toStdString());
            } else {
                throw std::runtime_error("Missing value for: " + fieldName.toStdString() + "; Of Type: " + fieldTypeName.toStdString());
            }
        }
    }
    return true;
}

} // namespace Settings
