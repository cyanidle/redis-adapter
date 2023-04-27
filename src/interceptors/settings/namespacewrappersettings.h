#ifndef RADAPTER_NAMESPACEWRAPPER_SETTINGS_H
#define RADAPTER_NAMESPACEWRAPPER_SETTINGS_H

#include "settings-parsing/serializablesetting.h"

namespace Settings {
struct NamespaceWrapper : Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<NestedKey>, wrap_into);
};
}

#endif // RADAPTER_NAMESPACEWRAPPER_H
