#ifndef REMAPPINGPIPESETTINGS_H
#define REMAPPINGPIPESETTINGS_H

#include "settings-parsing/serializablesetting.h"
namespace Settings {
struct FieldRemap : Serializable {
    Q_GADGET
    IS_SETTING
    FIELD(Required<QString>, from)
    FIELD(Required<QString>, to)
};

struct RemappingPipe : Serializable {
    Q_GADGET
    IS_SETTING
    FIELD(RequiredMapping<FieldRemap>, remaps)
};
}
#endif // REMAPPINGPIPESETTINGS_H
