#ifndef REMAPPINGPIPESETTINGS_H
#define REMAPPINGPIPESETTINGS_H

#include "settings-parsing/serializablesetting.h"
namespace Settings {
struct FieldRemap : Serializable {
    Q_GADGET
    IS_SETTING
    FIELD(Required<QVariant>, from)
    FIELD(Required<QVariant>, to)
};

struct RemappingPipe : Serializable {
    Q_GADGET
    IS_SETTING
    FIELD(RequiredMapping<FieldRemap>, remaps)
};
}
#endif // REMAPPINGPIPESETTINGS_H
