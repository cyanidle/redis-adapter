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
    Worker(const QString &name = "") : name(name) {}
    bool isValid() const {
        return !name->isEmpty();
    }
};

}

#endif // WORKERBASESETTINGS_H
