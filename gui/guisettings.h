#ifndef GUISETTINGS_H
#define GUISETTINGS_H

#include "settings-parsing/serializablesettings.h"

struct GuiSettings : Settings::SerializableSettings {
    Q_GADGET
    FIELDS(enabled)
    Settings::NonRequiredField<bool> enabled{false};
};


#endif // GUISETTINGS_H
