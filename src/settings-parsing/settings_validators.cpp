#include "settings_validators.h"

bool Settings::ChooseJsonFormat::validate(QVariant &src) {
    static QMap<QString, QJsonDocument::JsonFormat> map {
        {"compact", QJsonDocument::Compact},
        {"indented", QJsonDocument::Indented}
    };
    auto asStr = src.toString().toLower();
    src.setValue(map.value(asStr));
    return map.contains(asStr);
}

bool Settings::StringToFileSize::validate(QVariant &src) {
    auto asStr = src.toString().toUpper();
    if (asStr.isEmpty()) return true;
    auto letter = asStr.at(asStr.length() - 1);
    bool ok;
    auto number = asStr.left(asStr.length() - 1).toDouble(&ok);
    if (!ok) {
        throw std::runtime_error("Could not convert to number: " + asStr.toStdString());
    }
    if (letter == 'G') {
        number *= 1024 * 1024 * 1024;
    } else if (letter == 'M') {
        number *= 1024 * 1024;
    } else if (letter == 'K') {
        number *= 1024;
    } else {
        throw std::runtime_error("Invalid size char: " + QString(letter).toStdString() + "; In: " + asStr.toStdString());
    }
    src.setValue(qRound(number));
    return true;
}
