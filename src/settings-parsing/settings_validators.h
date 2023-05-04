#ifndef SETTINGS_VALIDATORS_H
#define SETTINGS_VALIDATORS_H

#include "settings-parsing/serializablesetting.h"

namespace Settings {
struct ChooseJsonFormat {
    static const QString &name();
    static bool validate(QVariant &src);
};
struct StringToFileSize {
    static const QString &name();
    static bool validate(QVariant &src);
};
using NonRequiredFileSize = ::Serializable::Validated<Settings::HasDefault<quint64>>::With<Settings::StringToFileSize>;
using RequiredFileSize = ::Serializable::Validated<Settings::Required<quint64>>::With<Settings::StringToFileSize>;
}


#endif // SETTINGS_VALIDATORS_H
