#ifndef LOGGINGINTERCEPTORSETTINGS_H
#define LOGGINGINTERCEPTORSETTINGS_H

#include <QJsonDocument>
#include "settings-parsing/serializablesettings.h"
#include "private/global.h"

Q_DECLARE_METATYPE(QJsonDocument::JsonFormat)
namespace Radapter {

struct RADAPTER_SHARED_SRC LoggingInterceptorSettings : public Settings::SerializableSettings  {
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
    SERIAL_FIELD(QString, filepath)
    SERIAL_FIELD(quint32, flush_delay, 1000u)
    SERIAL_FIELD(quint64, max_size_bytes, 100000000UL)
    SERIAL_FIELD(quint64, rotating, true)
    SERIAL_CUSTOM(LogMsgs, log, initLog, NO_READ, LogNormal)
    SERIAL_FIELD_MAPPED(QJsonDocument::JsonFormat, format, mapping(), QJsonDocument::Indented)
protected:
    static QMap<QString, QJsonDocument::JsonFormat> &mapping() {
        static QMap<QString, QJsonDocument::JsonFormat> map {
            {"compact", QJsonDocument::Compact},
            {"indented", QJsonDocument::Indented}
        };
        return map;
    }
    bool initLog(const QVariant &src) {
        const auto strRep = src.toString().toLower();
        if (strRep == "all") {
            log = LogAll;
        } else {
            const auto splitVersions = src.toList();
            if (splitVersions.contains("normal") || strRep == "normal") {
                log |= LogNormal;
            }
            if (splitVersions.contains("reply") || strRep == "reply") {
                log |= LogReply;
            }
            if (splitVersions.contains("command") || strRep == "command") {
                log |= LogCommand;
            }
        }
        if (!log) {
            return false;
        }
        return true;
    }
};
Q_DECLARE_OPERATORS_FOR_FLAGS(LoggingInterceptorSettings::LogMsgs)
}
#endif // LOGGINGINTERCEPTORSETTINGS_H
