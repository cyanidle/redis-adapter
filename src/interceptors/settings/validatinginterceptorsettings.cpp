#include "validatinginterceptorsettings.h"
#include "templates/algorithms.hpp"
#include <QRegularExpression>

void Settings::ValidatingInterceptor::init() {
    final_by_validator_glob.clear();
    final_by_validator.clear();
    QMap<QString, QStringList> temp;
    for (auto [validator, fields]: by_validator) {
        temp[validator].append(fields);
    }
    for (auto [field, val]: by_field) {
        temp[val].append(field);
    }
    for (auto [valName, fields]: Radapter::keyVal(temp)) {
        auto validator = Validator::Fetched(valName);
        for (const auto &field: fields) {
            final_by_validator[validator].append(field);
        }
    }
    for (auto [name, value]: by_glob) {
        auto validator = Validator::Fetched(name);
        auto asRegex = QRegularExpression::wildcardToRegularExpression(value);
        final_by_validator_glob.insert(validator, QRegularExpression{asRegex});
    }
}
