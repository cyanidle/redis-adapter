#ifndef LOGGINGINTERCEPTORSETTINGS_H
#define LOGGINGINTERCEPTORSETTINGS_H

#include <QJsonDocument>
#include "settings-parsing/serializablesetting.h"
#include "private/global.h"
#include "workersettings.h"
#include "settings-parsing/settings_validators.h"

namespace Settings {

struct RADAPTER_API FileWorker : public Serializable
{
    Q_GADGET
    IS_SETTING
    FIELD(Required<Worker>, worker)
    FIELD(Settings::Required<QString>, filepath)
    FIELD(VALIDATED(HasDefault<QJsonDocument::JsonFormat>, ChooseJsonFormat), format, QJsonDocument::Indented)

};

}
#endif // LOGGINGINTERCEPTORSETTINGS_H
