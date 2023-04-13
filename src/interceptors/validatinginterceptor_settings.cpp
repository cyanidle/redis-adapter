#include "validatinginterceptor_settings.h"
#include <QRegularExpression>

void Settings::ValidatingInterceptor::postUpdate() {
    QMap<QString, QStringList> temp;
    for (auto iter = by_validator->cbegin(); iter != by_validator->cend(); ++iter) {
        temp[iter.key()].append(iter.value());
    }
    for (auto iter = by_field->cbegin(); iter != by_field->cend(); ++iter) {
        temp[iter.value()].append(iter.key());
    }
    for (auto iter = temp.cbegin(); iter != temp.cend(); ++iter) {
        auto validator = Serializable::Validator(iter.key());
        for (const auto &field: iter.value()) {
            final_by_validator[validator].append(field);
        }
    }
    for (auto iter = by_glob.cbegin(); iter != by_glob.cend(); ++iter) {
        auto validator = Serializable::Validator(iter.key());
        auto asRegex = QRegularExpression::wildcardToRegularExpression(iter.value());
        final_by_validator_glob.insert(validator, QRegularExpression{asRegex});
    }
}
