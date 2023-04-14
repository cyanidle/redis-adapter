#ifndef WORKERBASESETTINGS_H
#define WORKERBASESETTINGS_H

#include "settings-parsing/serializablesettings.h"
namespace Settings {
struct RADAPTER_API Worker : public Settings::SerializableSettings
{
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<QString>, name)
    FIELD(NonRequiredLogLevel, log_level, QtMsgType::QtDebugMsg)
    FIELD(HasDefault<bool>, print_msgs, false)
    Worker(const QString &name = "") : name(name) {}
};

}

#endif // WORKERBASESETTINGS_H
