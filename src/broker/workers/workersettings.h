#ifndef WORKERBASESETTINGS_H
#define WORKERBASESETTINGS_H

#include "settings-parsing/serializablesettings.h"

namespace Radapter {

struct RADAPTER_API WorkerSettings : public Settings::SerializableSettings
{
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Settings::Required<QString>, name)
    FIELD(Settings::NonRequiredSequence<QString>, producers)
    FIELD(Settings::NonRequiredSequence<QString>, consumers)
    FIELD(Settings::NonRequired<bool>, print_msgs)
    WorkerSettings(const QString &name = "") : name(name) {}
};

}

#endif // WORKERBASESETTINGS_H
