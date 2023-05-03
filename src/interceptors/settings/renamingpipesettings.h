#ifndef RENAMINGPIPESETTINGS_H
#define RENAMINGPIPESETTINGS_H
#include "settings-parsing/serializablesetting.h"
namespace Settings {
struct RenamingPipe : Serializable {
    Q_GADGET
    IS_SETTING
    FIELD(RequiredMapping<QString>, renames)
};
}

#endif // RENAMINGPIPESETTINGS_H
