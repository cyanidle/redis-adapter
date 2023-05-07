#include "processworker.h"
#include "broker/workers/settings/processworkersettings.h"
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include "private/privfilehelper.h"

using namespace Radapter;

struct ProcessWorker::Private {
    Settings::ProcessWorker settings;
    QProcess *proc;
    ::Radapter::Private::FileHelper *outHelp;
};

ProcessWorker::ProcessWorker(const Settings::ProcessWorker &settings, QThread *thread) :
    Worker(settings.worker, thread),
    d(new Private{settings, new QProcess(this), nullptr})
{
    if (!exists(d->settings.process)) {
        throw std::runtime_error("Cannot find programm: " + d->settings.process->toStdString());
    }
    addPaths(d->proc);
    connect(d->proc, &QProcess::stateChanged, this, [this](QProcess::ProcessState st){
        if (st == QProcess::ProcessState::Running) {
            emit processStarted();
        }
        workerInfo(this) << "new process state:" << st;
    });
    d->proc->setReadChannel(QProcess::StandardOutput);
    d->outHelp = new ::Radapter::Private::FileHelper(d->proc, this);
    connect(d->outHelp, &::Radapter::Private::FileHelper::jsonRead, this, &ProcessWorker::send);
    connect(d->outHelp, &::Radapter::Private::FileHelper::error, this, [this](const QString &reason){
        workerError(this) << "Error reading process stdout, reason:" << reason;
    });
    connect(d->proc, &QProcess::finished, this, [this](int code, QProcess::ExitStatus st) {
        if (!code && st == QProcess::NormalExit) {
            workerInfo(this) << "Process finished with exit code: 0";
            emit finished(true);
            if (d->settings.restart_on_ok) {
                restart();
            }
        } else {
            workerError(this) << "Process finished abnormally! Code:" << code << "Stderr:" << d->proc->readAllStandardError();
            emit finished(false);
            if (d->settings.restart_on_fail) {
                restart();
            }
        }
    });
    connect(d->proc, &QProcess::readyReadStandardError, this, &ProcessWorker::onStderrReady);
}

void ProcessWorker::restart()
{
    workerWarn(this) << "Restarting in" << d->settings.restart_delay_ms / 1000. << "...";
    QTimer::singleShot(d->settings.restart_delay_ms, this, [this]{
        d->proc->start(d->settings.process, d->settings.arguments);
    });
}

ProcessWorker::~ProcessWorker()
{
    delete d;
}

QProcess *ProcessWorker::underlying()
{
    return d->proc;
}

bool ProcessWorker::exists(QString proc)
{
    QProcess findProcess;
    addPaths(&findProcess);
    QStringList arguments;
    arguments << proc;
#ifdef Q_OS_UNIX
    findProcess.start("which", arguments);
#elif defined(Q_OS_WINDOWS)
    findProcess.start("WHERE", arguments);
#else
#error "Unsupported platform!"
#endif
    findProcess.setReadChannel(QProcess::StandardOutput);
    if(!findProcess.waitForFinished())
        return false; // Not found or which does not work
    QString retStr(findProcess.readAll());
    retStr = retStr.trimmed();
    QFile file(retStr);
    QFileInfo check_file(file);
    if (check_file.exists() && check_file.isFile())
        return true; // Found!
    else
        return false; // Not found!
}

void ProcessWorker::onRun()
{
    d->proc->start(d->settings.process, d->settings.arguments);
}

void ProcessWorker::onMsg(const WorkerMsg &msg)
{
    d->proc->write(msg.json().toBytes());
    d->proc->write("\r\n");
}

void ProcessWorker::addPaths(QProcess *proc)
{
    auto was = QProcessEnvironment::systemEnvironment();
    auto wasPath = was.value("PATH");
    QString toPrepend;
#ifdef Q_OS_UNIX
    toPrepend = d->settings.extra_paths->join(":");
#elif defined(Q_OS_WINDOWS)
    toPrepend = d->settings.extra_paths->join(";");
#else
#error "Unsupported platform!"
#endif
    was.insert("PATH", toPrepend + wasPath);
    proc->setProcessEnvironment(was);
}

void ProcessWorker::onStderrReady()
{
    auto data = d->proc->readAllStandardError();
    data.resize(data.length() - 1);
    workerInfo(this) << data;
}

void ProcessWorker::onProcStarted()
{
    QIODevice::OpenMode mode;
    if (d->settings.read) {
        mode |= QIODevice::ReadOnly;
    }
    if (d->settings.write) {
        mode |= QIODevice::WriteOnly;
    }
    if (!d->proc->open(mode)) {
        workerError(this) << "Could not open process pipes! Open Mode:" << mode;
    }
    d->outHelp->start();
}
