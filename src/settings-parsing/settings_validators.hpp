#ifndef SETTINGS_VALIDATORS_HPP
#define SETTINGS_VALIDATORS_HPP

#include "settings-parsing/serializablesettings.h"

namespace Settings {
struct ChooseJsonFormat {
    static bool validate(QVariant &src) {
        static QMap<QString, QJsonDocument::JsonFormat> map {
            {"compact", QJsonDocument::Compact},
            {"indented", QJsonDocument::Indented}
        };
        auto asStr = src.toString().toLower();
        src.setValue(map.value(asStr));
        return map.contains(asStr);
    }
};
struct StringToFileSize {
    static bool validate(QVariant &src) {
        auto asStr = src.toString().toLower();
        if (asStr.isEmpty()) return true;
        auto letter = asStr.at(asStr.length() - 1);
        bool ok;
        auto number = asStr.left(asStr.length() - 2).toLongLong(&ok);
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
        src.setValue(number);
        return true;
    }
};
using NonRequiredFileSize = Serializable::Validate<Settings::NonRequired<quint64>, Settings::StringToFileSize>;
using RequiredFileSize = Serializable::Validate<Settings::Required<quint64>, Settings::StringToFileSize>;
}


#endif // SETTINGS_VALIDATORS_HPP
