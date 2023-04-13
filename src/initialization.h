#ifndef INITIALIZATION_H
#define INITIALIZATION_H

#include "private/global.h"
#include "radapterlogging.h"

class QLibrary;
class QStringList;

namespace Radapter {

void initPipelines(const QStringList &pipelines);
void initPlugins(const QList<QLibrary*> plugins);

template <typename Callable, typename...Args>
void tryInit(Callable callable, const QString &moduleName, Args&&...args) {
    try {
        callable(std::forward<Args>(args)...);
    } catch(std::runtime_error &exc) {
        settingsParsingWarn().nospace() << "Could not enable module: [" << moduleName.toUpper() << "] --> Details: " << exc.what();
    }
}

}

#endif // INITIALIZATION_H
