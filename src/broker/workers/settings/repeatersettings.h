#ifndef REPEATER_SETTINGS_H
#define REPEATER_SETTINGS_H
#include "workersettings.h"
#include "settings-parsing/serializablesetting.h"
namespace Settings {
struct Repeater : Worker {
    Q_GADGET
    IS_SERIALIZABLE
};
}


#endif //REPEATER_SETTINGS_H
