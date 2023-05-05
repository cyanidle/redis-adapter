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
    QProcess *underlying();
    bool exists(QString proc);
    void onRun() override;
signals:
    void processStarted();
public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
protected:
    void addPaths(QProcess *proc);
private slots:
    void onStderrReady();
    void onProcStarted();
private:
    Private *d;
};

} // namespace Radapter

#endif // RADAPTER_PROCESSWORKER_H
