#ifndef GUISETTINGS_H
#define GUISETTINGS_H

#include "settings-parsing/serializablesettings.h"

namespace Gui {

struct GuiSettings : Settings::SerializableSettings {
    Q_GADGET
    FIELDS(enabled)
    Settings::NonRequiredField<bool> enabled{false};
};

} // namespace Gui

#endif // GUISETTINGS_H
