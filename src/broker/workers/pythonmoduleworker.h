#ifndef RADAPTER_PYTHONMODULEWORKER_H
#define RADAPTER_PYTHONMODULEWORKER_H

#include "worker.h"
namespace Settings{struct PythonModuleWorker;}
namespace Radapter {

class PythonModuleWorker : public Radapter::Worker
{
    Q_OBJECT
    struct Private;
public:
    PythonModuleWorker(const Settings::PythonModuleWorker &settings, QThread *thread);
    void onRun() override;
public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
private:
    Private *d;
};

} // namespace Radapter

#endif // RADAPTER_PYTHONMODULEWORKER_H
