#ifndef RADAPTER_NAMESPACEUNWRAPPER_SETTINGS_H
#define RADAPTER_NAMESPACEUNWRAPPER_SETTINGS_H

#include "settings-parsing/serializablesetting.h"

namespace Settings {
struct NamespaceUnwrapper : Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<NestedKey>, unwrap_from);
    FIELD(HasDefault<bool>, print_filtered, true);
};
}

#endif // RADAPTER_NAMESPACEUNWRAPPER_H
