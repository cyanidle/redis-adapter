#ifndef LOGGINGINTERCEPTORSETTINGS_H
#define LOGGINGINTERCEPTORSETTINGS_H

#include <QJsonDocument>
#include "settings-parsing/serializablesettings.h"
#include "private/global.h"
#include "workersettings.h"
#include "settings-parsing/settings_validators.h"

namespace Settings {

struct RADAPTER_API LoggingWorker : public Worker
{
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
    FIELD(Settings::NonRequired<QString>, log, "normal")
    using NonRequiredJsonFormat = Serializable::Validated<Settings::NonRequired<QJsonDocument::JsonFormat>>::With<Settings::ChooseJsonFormat>;
    FIELD(NonRequiredJsonFormat, format, QJsonDocument::Indented)

    LogMsgs log_{LogNormal};
protected:
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
Q_DECLARE_OPERATORS_FOR_FLAGS(LoggingWorker::LogMsgs)
}
#endif // LOGGINGINTERCEPTORSETTINGS_H
