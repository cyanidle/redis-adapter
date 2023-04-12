#ifndef VALIDATING_INTERCEPTOR_SETTINGS_H
#define VALIDATING_INTERCEPTOR_SETTINGS_H

#include "settings-parsing/serializablesettings.h"

namespace Settings {
struct ValidatingInterceptor : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(NonRequiredMapping<QString>, by_field)
    FIELD(NonRequiredMapping<QStringList>, by_validator)
    FIELD(NonRequired<bool>, validate_incoming, false)

    QMap<Serializable::Validator, QStringList> final_by_validator;

    POST_UPDATE {
        QMap<QString, QStringList> temp;
        for (auto iter = by_validator->cbegin(); iter != by_validator->cend(); ++iter) {
            temp[iter.key()].append(iter.value());
        }
        for (auto iter = by_field->cbegin(); iter != by_field->cend(); ++iter) {
            temp[iter.value()].append(iter.key());
        }
        for (auto iter = temp.cbegin(); iter != temp.cend(); ++iter) {
            final_by_validator.insert({iter.key()}, iter.value());
        }
    }
};
}

#endif // VALIDATING_INTERCEPTOR_SETTINGS_H
