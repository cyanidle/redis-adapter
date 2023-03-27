#ifndef GUISETTINGS_H
#define GUISETTINGS_H

#include "settings-parsing/serializablesettings.h"

namespace Gui {

struct GuiSettings : Settings::SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Settings::NonRequired<bool>, enabled, {false})
};

} // namespace Gui

#endif // GUISETTINGS_H
