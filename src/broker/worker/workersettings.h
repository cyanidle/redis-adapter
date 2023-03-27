#ifndef WORKERBASESETTINGS_H
#define WORKERBASESETTINGS_H

#include "settings-parsing/serializablesettings.h"

namespace Radapter {

struct RADAPTER_API WorkerSettings : public Settings::SerializableSettings {
    Q_GADGET
    FIELDS(name, producers, consumers, print_msgs)
    Settings::RequiredField<QString> name;
    Settings::NonRequiredSequence<QString> producers;
    Settings::NonRequiredSequence<QString> consumers;
    Settings::NonRequiredField<bool> print_msgs;
    WorkerSettings(const QString &name = "") : name(name) {}
};

}

#endif // WORKERBASESETTINGS_H
