#ifndef RADAPTER_NAMESPACEUNWRAPPER_SETTINGS_H
#define RADAPTER_NAMESPACEUNWRAPPER_SETTINGS_H

#include "settings-parsing/serializablesetting.h"

namespace Settings {
struct NamespaceUnwrapper : Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<NestedKey>, unwrap_from);
};
}

#endif // RADAPTER_NAMESPACEUNWRAPPER_H
