#ifndef BROKERSETTINGS_H
#define BROKERSETTINGS_H

#include "settings-parsing/serializablesetting.h"

namespace Settings {
struct Broker : Serializable
{
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Settings::HasDefault<bool>, warn_no_receivers, true)
    FIELD(Settings::HasDefault<bool>, allow_self_connect, false)
};
}
#endif // BROKERSETTINGS_H
