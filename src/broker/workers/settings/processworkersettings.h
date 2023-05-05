#ifndef PROCESSWORKERSETTINGS_H
#define PROCESSWORKERSETTINGS_H
#include "workersettings.h"
#include "settings-parsing/serializablesetting.h"
namespace Settings {
struct ProcessWorker : Serializable {
    Q_GADGET
    IS_SETTING
    FIELD(Required<Worker>, worker)
    FIELD(Required<QString>, process)
    COMMENT(process, "Executable in PATH or /full/path/to/exec")
    FIELD(OptionalSequence<QString>, extra_paths)
    COMMENT(extra_paths, "Additional directories to seek for executable")
    FIELD(OptionalSequence<QString>, arguments)
    COMMENT(arguments, "Arguments in format of [-v, --verbose, positional_arg, -f, file, etc...]")
};
}
#endif // PROCESSWORKERSETTINGS_H
