#ifndef RADAPTER_PROCESSWORKER_H
#define RADAPTER_PROCESSWORKER_H

#include "worker.h"
class QProcess;
namespace Settings{struct ProcessWorker;}
namespace Radapter {
class ProcessWorker : public Radapter::Worker
{
    Q_OBJECT
    struct Private;
public:
    ProcessWorker(const Settings::ProcessWorker &settings, QThread *thread);
    ~ProcessWorker() override;
    bool exists(QString proc);
    void onRun() override;
public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
protected:
    void addPaths(QProcess *proc);
private:
    Private *d;
};

} // namespace Radapter

#endif // RADAPTER_PROCESSWORKER_H
