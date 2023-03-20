#ifndef LOGGINGINTERCEPTORSETTINGS_H
#define LOGGINGINTERCEPTORSETTINGS_H

#include <QJsonDocument>
#include "settings-parsing/serializablesettings.h"
#include "private/global.h"

namespace Radapter {

struct RADAPTER_API LoggingInterceptorSettings : public Settings::SerializableSettings  {
    Q_GADGET
    FIELDS(filepath, flush_delay, max_size_bytes, rotating)
    enum LogMsgTypes {
        LogNone = 0,
        LogAll = 0x0001,
        LogNormal = 0x0002,
        LogReply = 0x0004,
        LogCommand = 0x0008
    };
    Q_DECLARE_FLAGS(LogMsgs, LogMsgTypes)
    Q_ENUM(LogMsgTypes)
    Settings::RequiredField<QString> filepath;
    Settings::NonRequiredField<quint32> flush_delay{1000u};
    Settings::NonRequiredField<quint64> max_size_bytes{100000000UL};
    Settings::NonRequiredField<quint64> rotating{true};
    Settings::NonRequiredField<QString> log{"normal"};
    LogMsgs log_{LogNormal};
    using JsonField = Serializable::Validated<Settings::NonRequiredField<QJsonDocument::JsonFormat>>::With<Settings::ChooseJsonFormat>;
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
