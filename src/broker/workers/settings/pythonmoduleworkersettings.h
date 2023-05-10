#ifndef PYTHONMODULEWORKERSETTINGS_H
#define PYTHONMODULEWORKERSETTINGS_H
#include "settings-parsing/serializablesetting.h"
#include "workersettings.h"
namespace Settings {

struct PyDebug : Serializable {
    Q_GADGET
    IS_SETTING
    FIELD(HasDefault<bool>, enabled, false)
    FIELD(HasDefault<quint32>, port, 5678)
    FIELD(HasDefault<bool>, wait, false)
};

struct PythonModuleWorker : Serializable {
    Q_GADGET
    IS_SETTING
    FIELD(Required<Worker>, worker)
    FIELD(Required<QString>, module_path)
    FIELD(Optional<QVariantMap>, module_settings)
    FIELD(OptionalSequence<QString>, extra_paths)
    FIELD(Optional<PyDebug>, debug)
};
}
#endif // PYTHONMODULEWORKERSETTINGS_H
