#include "loggingworker.h"
#include "radapterlogging.h"
#include <QDir>
#include "broker/workers/worker.h"
#include <QDateTime>

using namespace Radapter;

LoggingWorker::LoggingWorker(const LoggingWorkerSettings &settings, QThread *thread) :
    Radapter::Worker(settings, thread),
    m_file(new QFile(settings.filepath, this)),
    m_settings(settings)
{
    QDir().mkpath(settings.filepath->left(settings.filepath->lastIndexOf("/")));
    if (!m_file->open(QIODevice::Append | QIODevice::WriteOnly | QIODevice::Text)) {
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
    return m_settings.filepath;
}

QString LoggingWorker::currentFilepath() const
{
    return m_file->fileName();
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
    if (!m_file->isOpen()) {
        if (!m_file->open(QIODevice::Append | QIODevice::Text | QIODevice::WriteOnly)) {
            workerError(this) << "Could not open file with name: " << m_file->fileName();
            return;
        }
    }
    QTextStream out(m_file);
    out << info.toBytes(m_settings.format) << ",";
    Qt::endl(out);
}

bool LoggingWorker::testMsgForLog(const Radapter::WorkerMsg &msg) {
    if (m_settings.log_.testFlag(LoggingWorkerSettings::LogAll)) {
        return true;
    }
    if (!msg.isCommand() && !msg.isReply())
        return m_settings.log_.testFlag(
            LoggingWorkerSettings::LogNormal);
    if (msg.isReply())
        return m_settings.log_.testFlag(
            LoggingWorkerSettings::LogReply);
    if (msg.isCommand())
        return m_settings.log_.testFlag(
            LoggingWorkerSettings::LogCommand);
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
