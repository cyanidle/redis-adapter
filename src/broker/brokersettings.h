#ifndef BROKERSETTINGS_H
#define BROKERSETTINGS_H

#include "settings-parsing/serializablesettings.h"

namespace Radapter {
struct BrokerSettings : Settings::SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Settings::NonRequired<bool>, warn_no_receivers, true)
};
}
#endif // BROKERSETTINGS_H
