#ifndef RADAPTER_PYTHONMODULEWORKER_H
#define RADAPTER_PYTHONMODULEWORKER_H

#include "worker.h"
namespace Settings{struct PythonModuleWorker;}
namespace Radapter {

class PythonModuleWorker : public Radapter::Worker
{
    Q_OBJECT
public:
    PythonModuleWorker(const Settings::PythonModuleWorker &settings, QThread *thread);
};

} // namespace Radapter

#endif // RADAPTER_PYTHONMODULEWORKER_H
