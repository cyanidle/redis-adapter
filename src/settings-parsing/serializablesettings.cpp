#include "serializablesettings.h"
#include "jsondict/jsondict.h"
#include "templates/algorithms.hpp"

namespace Settings {

QString SerializableSettings::print() const
{
    return JsonDict{serialize()}.printDebug().replace("Json", metaObject()->className());
}

bool SerializableSettings::update(const QVariantMap &src)
{
    auto selfName = std::string(metaObject()->className());
    for (auto key = src.keyBegin(); key != src.keyEnd(); ++key) {
        if (!fields().contains(*key)) {
            throw std::runtime_error("Extra field passed to Serializable Setting: "
                                     + std::string(metaObject()->className())
                                     + " --> " + key->toStdString());
        }
    }
    for (const auto &fieldName : fields()) {
        auto found = field(fieldName);
        auto valueFromSource = src.value(fieldName);
        auto required = !found->attributes(this).contains("non_required");
        if (!valueFromSource.isValid() && !required) continue;
        if (!found->updateWithVariant(this, valueFromSource) && required) {
            auto fieldTypeName = QStringLiteral("%1<%2>").arg(found->typeName(this), QString(QMetaType(found->valueMetaTypeId(this)).name()));
            if (src.contains(fieldName)) {
                auto msg = QStringLiteral("Wanted: %1; Received: %2").arg(fieldTypeName, src[fieldName].toString());
                throw std::runtime_error(selfName + ": Value type missmatch: " + msg.toStdString());
            } else {
                throw std::runtime_error(selfName + ": Missing value for: " + fieldName.toStdString() + "; Of Type: " + fieldTypeName.toStdString());
            }
        }
    }
    postUpdate();
    return true;
}

} // namespace Settings
