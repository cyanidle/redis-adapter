#ifndef WORKERBASESETTINGS_H
#define WORKERBASESETTINGS_H

#include "settings-parsing/serializablesettings.h"

namespace Radapter {

struct RADAPTER_API WorkerSettings : public Settings::SerializableSettings {
    Q_GADGET
    FIELDS(name, producers, consumers, print_msgs)
    Serializable::Field<QString> name;
    Settings::NonRequired<Serializable::Sequence<QString>> producers;
    Settings::NonRequired<Serializable::Sequence<QString>> consumers;
    Settings::NonRequired<Serializable::Field<bool>> print_msgs;
};

}

#endif // WORKERBASESETTINGS_H
