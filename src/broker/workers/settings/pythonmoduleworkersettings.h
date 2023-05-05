#ifndef PYTHONMODULEWORKERSETTINGS_H
#define PYTHONMODULEWORKERSETTINGS_H
#include "settings-parsing/serializablesetting.h"
#include "workersettings.h"
namespace Settings {
struct PythonModuleWorker : Serializable {
    Q_GADGET
    IS_SETTING
    FIELD(Required<Worker>, worker)
    FIELD(Required<QString>, module_path)
    FIELD(Optional<QVariantMap>, module_settings)
    FIELD(OptionalSequence<QString>, extra_paths)
};
}
#endif // PYTHONMODULEWORKERSETTINGS_H
