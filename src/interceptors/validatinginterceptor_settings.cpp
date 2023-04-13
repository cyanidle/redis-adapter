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
        QList<QRegularExpression> regs;
        auto validator = Serializable::Validator(iter.key());
        for (const auto &field: iter.value()) {
            if (field.contains('*') || field.contains('!')) {
                regs.append(QRegularExpression{QRegularExpression::wildcardToRegularExpression(field)});
                regs.last().optimize();
            } else {
                final_by_validator[validator].append(field);
            }
        }
        if (!regs.isEmpty()) {
            final_by_validator_globs.insert({iter.key()}, regs);
        }
    }
}
