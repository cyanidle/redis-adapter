#ifndef VALIDATINGINTERCEPTOR_SETTINGS_H
#define VALIDATINGINTERCEPTOR_SETTINGS_H

#include "settings-parsing/serializablesetting.h"
#include <QRegularExpression>

namespace Settings {
struct ValidatingInterceptor : public Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalMapping<QString>, by_field)
    FIELD(OptionalMapping<QStringList>, by_validator)
    FIELD(OptionalMapping<QString>, by_glob)
    FIELD(HasDefault<bool>, inverse, false)
    COMMENT(inverse, "'Inverse mode' applies validators to all field EXCEPT ones defined in other fields")
    struct FieldPair {
        QString joined;
        QStringList full;
    };
    QMap<Validator::Fetched, QList<FieldPair>> final_by_validator;
    QMap<Validator::Fetched, QRegularExpression> final_by_validator_glob;
    void init();
};
}

#endif // VALIDATINGINTERCEPTOR_SETTINGS_H
