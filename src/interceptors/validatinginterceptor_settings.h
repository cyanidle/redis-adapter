#ifndef VALIDATINGINTERCEPTOR_SETTINGS_H
#define VALIDATINGINTERCEPTOR_SETTINGS_H

#include "settings-parsing/serializablesettings.h"
#include <QRegularExpression>

namespace Settings {
struct ValidatingInterceptor : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(NonRequiredMapping<QString>, by_field)
    FIELD(NonRequiredMapping<QStringList>, by_validator)

    QMap<Serializable::Validator, QList<QRegularExpression>> final_by_validator_globs;
    QMap<Serializable::Validator, QStringList> final_by_validator;
    void postUpdate() override;
};
}

#endif // VALIDATINGINTERCEPTOR_SETTINGS_H
