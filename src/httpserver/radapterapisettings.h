#ifndef RADAPTERAPISETTINGS_H
#define RADAPTERAPISETTINGS_H

#include "broker/workers/settings/workersettings.h"
#include "settings-parsing/settings_validators.h"

namespace Settings {
struct RadapterApi : public SerializableSettings
{
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(HasDefault<QString>, host, "0.0.0.0")
    FIELD(HasDefault<quint16>, port, 8080)
    FIELD(HasDefault<bool>, enabled, false)
    using JsonFormatHasDefault = Serializable::Validated<Settings::HasDefault<QJsonDocument::JsonFormat>>::With<Settings::ChooseJsonFormat>;
    FIELD(JsonFormatHasDefault, json_format, QJsonDocument::Indented)
};
}


#endif // RADAPTERAPISETTINGS_H
