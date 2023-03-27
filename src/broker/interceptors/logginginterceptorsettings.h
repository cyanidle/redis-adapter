#ifndef LOGGINGINTERCEPTORSETTINGS_H
#define LOGGINGINTERCEPTORSETTINGS_H

#include <QJsonDocument>
#include "settings-parsing/serializablesettings.h"
#include "private/global.h"

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
}

namespace Radapter {

struct RADAPTER_API LoggingInterceptorSettings : public Settings::SerializableSettings  {
    Q_GADGET
    IS_SERIALIZABLE
    enum LogMsgTypes {
        LogNone = 0,
        LogAll = 0x0001,
        LogNormal = 0x0002,
        LogReply = 0x0004,
        LogCommand = 0x0008
    };
    Q_DECLARE_FLAGS(LogMsgs, LogMsgTypes)
    Q_ENUM(LogMsgTypes)
    FIELD(Settings::Required<QString>, filepath)
    FIELD(Settings::NonRequired<quint32>, flush_delay, {1000u})
    FIELD(Settings::NonRequired<quint64>, max_size_bytes, {100000000UL})
    FIELD(Settings::NonRequired<quint64>, rotating, {true})
    FIELD(Settings::NonRequired<QString>, log, {"normal"})

    LogMsgs log_{LogNormal};
    using JsonField = Serializable::Validated<Settings::NonRequired<QJsonDocument::JsonFormat>>::With<Settings::ChooseJsonFormat>;
    JsonField format{QJsonDocument::Indented};
protected:
    static QMap<QString, QJsonDocument::JsonFormat> &mapping() {
        static QMap<QString, QJsonDocument::JsonFormat> map {
            {"compact", QJsonDocument::Compact},
            {"indented", QJsonDocument::Indented}
        };
        return map;
    }
    POST_UPDATE {
        const auto strRep = log->toLower();
        if (strRep == "all") {
            log_ = LogAll;
        } else {
            const auto splitVersions = log->split(" | ");
            if (splitVersions.contains("normal") || strRep == "normal") {
                log_ |= LogNormal;
            }
            if (splitVersions.contains("reply") || strRep == "reply") {
                log_ |= LogReply;
            }
            if (splitVersions.contains("command") || strRep == "command") {
                log_ |= LogCommand;
            }
        }
    }
};
Q_DECLARE_OPERATORS_FOR_FLAGS(LoggingInterceptorSettings::LogMsgs)
}
#endif // LOGGINGINTERCEPTORSETTINGS_H
