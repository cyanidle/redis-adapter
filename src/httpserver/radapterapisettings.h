#ifndef RADAPTERAPISETTINGS_H
#define RADAPTERAPISETTINGS_H

#include "broker/workers/workersettings.h"
#include "settings-parsing/settings_validators.h"

namespace Settings {
struct RadapterApi : public SerializableSettings
{
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(NonRequired<QString>, host, "0.0.0.0")
    FIELD(NonRequired<quint16>, port, 8080)
    FIELD(NonRequired<bool>, enable, true)
    using NonRequiredJsonFormat = Serializable::Validated<Settings::NonRequired<QJsonDocument::JsonFormat>>::With<Settings::ChooseJsonFormat>;
    FIELD(NonRequiredJsonFormat, json_format, QJsonDocument::Indented)
};
}


#endif // RADAPTERAPISETTINGS_H
