#include "loggingworker.h"
#include "broker/workers/loggingworkersettings.h"
#include "broker/workers/private/workermsg.h"
#include "jsondict/jsondict.h"
#include "radapterlogging.h"
#include <QDir>
#include "broker/workers/worker.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMutex>
#include <QFile>
#include <QTimer>

using namespace Radapter;

struct Radapter::LoggingWorkerPrivate
{
    QFile *file;
    Settings::LoggingWorker settings;
};

LoggingWorker::LoggingWorker(const Settings::LoggingWorker &settings, QThread *thread) :
    Radapter::Worker(settings, thread),
    d(new LoggingWorkerPrivate{
        new QFile(this),
        settings
    })
{
    QDir().mkpath(settings.filepath->left(settings.filepath->lastIndexOf("/")));
    if (!d->file->open(QIODevice::Append | QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error(std::string("Could not open file: ") + baseFilepath().toStdString());
    }
    auto onOpen = JsonDict{};
    onOpen["__meta__"] = JsonDict{
        {"started", QDateTime::currentDateTime().toString()}
    }.toVariant();
    appendToFile(onOpen);
}

const QString &LoggingWorker::baseFilepath() const
{
    return d->settings.filepath;
}

QString LoggingWorker::currentFilepath() const
{
    return d->file->fileName();
}

LoggingWorker::~LoggingWorker()
{
    delete d;
}

void LoggingWorker::onCommand(const WorkerMsg &msg)
{
    onMsg(msg);
}

void LoggingWorker::onReply(const WorkerMsg &msg)
{
    onMsg(msg);
}

void LoggingWorker::appendToFile(const JsonDict &info)
{
    if (!d->file->isOpen()) {
        if (!d->file->open(QIODevice::Append | QIODevice::Text | QIODevice::WriteOnly)) {
            workerError(this) << "Could not open file with name: " << d->file->fileName();
            return;
        }
    }
    QTextStream out(d->file);
    out << info.toBytes(d->settings.format) << ",";
    Qt::endl(out);
}

bool LoggingWorker::testMsgForLog(const Radapter::WorkerMsg &msg) {
    if (d->settings.log_.testFlag(Settings::LoggingWorker::LogAll)) {
        return true;
    }
    if (!msg.isCommand() && !msg.isReply())
        return d->settings.log_.testFlag(
            Settings::LoggingWorker::LogNormal);
    if (msg.isReply())
        return d->settings.log_.testFlag(
            Settings::LoggingWorker::LogReply);
    if (msg.isCommand())
        return d->settings.log_.testFlag(
            Settings::LoggingWorker::LogCommand);
    return true;
}

void LoggingWorker::onMsg(const Radapter::WorkerMsg &msg)
{
    JsonDict copy = msg;
    JsonDict metaInfo{QVariantMap{
        {"sender", msg.sender()->printSelf()},
        {"receivers", msg.printReceivers()},
        {"flags", msg.printFlags()},
        {"command", msg.command() ? msg.command()->metaObject()->className() : "None"},
        {"reply", msg.reply() ? msg.reply()->metaObject()->className() : "None"},
        {"serviceInfo", msg.printServiceData().toVariant()},
        {"timestamp", QDateTime::currentDateTime().toString()},
        {"id", msg.id()}
    }};
    copy["__meta__"] = metaInfo.toVariant();
    appendToFile(copy);
}
