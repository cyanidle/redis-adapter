#ifndef WORKERBASESETTINGS_H
#define WORKERBASESETTINGS_H

#include "settings-parsing/serializablesetting.h"

namespace Settings {
struct RADAPTER_API Worker : public Settings::Serializable
{
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<QString>, name)
    FIELD(VALIDATED(HasDefault<QtMsgType>, Validator::LogLevel), log_level, QtMsgType::QtDebugMsg)
    FIELD(HasDefault<bool>, print_msgs, false)

    COMMENT(print_msgs, "Print all outgoing and incoming messages")
    COMMENT(name, "Name used in pipelines, e.g.: name > *pipe > name.2")
    COMMENT(log_level, "workerInfo(), workerWarn()... macros to enable")
    CLASS_COMMENT(Worker, "Worker part of config used by broker for routing")
    Worker(const QString &name = {}) : name(name) {}
};

}

#endif // WORKERBASESETTINGS_H
