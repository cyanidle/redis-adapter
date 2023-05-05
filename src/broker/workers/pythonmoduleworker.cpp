#include "pythonmoduleworker.h"
#include "broker/workers/processworker.h"
#include "broker/workers/settings/processworkersettings.h"
#include "broker/workers/settings/pythonmoduleworkersettings.h"
#include <QFile>
#include <QProcess>
#include<QResource>

using namespace Radapter;

struct PythonModuleWorker::Private {
    Settings::PythonModuleWorker settings;
    ProcessWorker *proc;
    QFile *bootstrap;
};

PythonModuleWorker::PythonModuleWorker(const Settings::PythonModuleWorker &settings, QThread *thread) :
    Worker(settings.worker, thread),
    d(new Private{settings, nullptr, nullptr})
{
    d->bootstrap = new QFile(":/py/bootstrap", this);
    Settings::ProcessWorker config;
    config.worker = settings.worker;
    config.worker->name.value += ".py";
    config.worker->print_msgs = false;
    config.process = "python3";
    config.extra_paths = settings.extra_paths;
    config.arguments.value = {"-",
                            "--settings", JsonDict(d->settings.module_settings.value).toBytes(),
                            "--name", d->settings.worker->name,
                            "--file", d->settings.module_path};
    d->proc = new ProcessWorker(config, thread);
    if (!d->bootstrap->open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Could not load bootstrap! Do not forget to Q_INIT_RESOURCE(radapter) in main if linking!");
    }
    connect(d->proc, &ProcessWorker::sendMsg, this, &PythonModuleWorker::sendMsg);
    connect(d->proc, &ProcessWorker::processStarted, this, [this]{
        auto toWr = d->bootstrap->readAll();
        d->proc->underlying()->write(toWr);
        d->proc->underlying()->closeWriteChannel();
    });
    d->proc->prepareForNested();
}

void PythonModuleWorker::onRun()
{
    d->proc->nestedRun();
}

void PythonModuleWorker::onMsg(const WorkerMsg &msg)
{
    d->proc->onMsg(msg);
}
