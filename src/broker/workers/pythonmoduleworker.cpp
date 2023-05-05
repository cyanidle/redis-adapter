#include "pythonmoduleworker.h"
#include "broker/workers/settings/pythonmoduleworkersettings.h"

using namespace Radapter;

PythonModuleWorker::PythonModuleWorker(const Settings::PythonModuleWorker &settings, QThread *thread) :
    Worker(settings.worker, thread)
{

}
