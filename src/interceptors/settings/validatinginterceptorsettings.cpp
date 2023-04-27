#include "validatinginterceptorsettings.h"
#include <QRegularExpression>

void Settings::ValidatingInterceptor::init() {
    final_by_validator_glob.clear();
    final_by_validator.clear();
    QMap<QString, QStringList> temp;
    for (auto iter = by_validator->cbegin(); iter != by_validator->cend(); ++iter) {
        temp[iter.key()].append(iter.value());
    }
    for (auto iter = by_field->cbegin(); iter != by_field->cend(); ++iter) {
        temp[iter.value()].append(iter.key());
    }
    for (auto iter = temp.cbegin(); iter != temp.cend(); ++iter) {
        auto validator = Validator::Fetched(iter.key());
        for (const auto &field: iter.value()) {
            final_by_validator[validator].append(field);
        }
    }
    for (auto [name, value]: by_glob) {
        auto validator = Validator::Fetched(name);
        auto asRegex = QRegularExpression::wildcardToRegularExpression(value);
        final_by_validator_glob.insert(validator, QRegularExpression{asRegex});
    }
}
