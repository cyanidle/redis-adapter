#ifndef VALIDATINGINTERCEPTOR_SETTINGS_H
#define VALIDATINGINTERCEPTOR_SETTINGS_H

#include "settings-parsing/serializablesettings.h"
#include <QRegularExpression>

namespace Settings {
struct ValidatingInterceptor : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(MappingHasDefault<QString>, by_field)
    FIELD(MappingHasDefault<QStringList>, by_validator)
    FIELD(MappingHasDefault<QString>, by_glob)

    QMap<Validator::Fetched, QStringList> final_by_validator;
    QMap<Validator::Fetched, QRegularExpression> final_by_validator_glob;
    void init();
};
}

#endif // VALIDATINGINTERCEPTOR_SETTINGS_H
