#include "loggingworker.h"
#include "radapterlogging.h"
#include <QDir>
#include "broker/workers/worker.h"
#include <QDateTime>

using namespace Radapter;

LoggingWorker::LoggingWorker(const LoggingWorkerSettings &settings, QThread *thread) :
    Radapter::Worker(settings, thread),
    m_file(new QFile(settings.filepath, this)),
    m_flushTimer(new QTimer(this)),
    m_settings(settings),
    m_array()
{
    connect(m_flushTimer, &QTimer::timeout, this, &LoggingWorker::onFlush);
    QDir().mkpath(settings.filepath->left(settings.filepath->lastIndexOf("/")));
    m_file->open(QIODevice::ReadOnly | QIODevice::Text);
    QJsonParseError err;
    QTextStream in(m_file);
    const QJsonDocument inDoc = QJsonDocument::fromJson(in.readAll().toUtf8(), &err);
    if (err.error != QJsonParseError::NoError && m_file->exists() && m_file->size()) {
        throw std::runtime_error(std::string("Will not overwrite contents of file: ") + baseFilepath().toStdString());
    }
    m_array = inDoc.array();
    auto onOpen = JsonDict{};
    onOpen["__meta__"] = JsonDict{
        {"started", QDateTime::currentDateTime().toString()}
    }.toVariant();
    m_array.append(onOpen.toJsonObj());
    m_file->close();
    m_flushTimer->start(settings.flush_delay);
}

bool LoggingWorker::isFull() const
{
    return static_cast<quint64>(m_file->size()) > m_settings.max_filesize;
}

void LoggingWorker::onCommand(const WorkerMsg &msg)
{
    onMsg(msg);
}

void LoggingWorker::onReply(const WorkerMsg &msg)
{
    onMsg(msg);
}

void LoggingWorker::onFlush()
{
    if (!m_shouldUpdate) {
        return;
    }
    m_shouldUpdate = false;
    if (!m_file->isOpen()) {
        if (!m_file->open(QIODevice::Append | QIODevice::Text | QIODevice::WriteOnly)) {
            workerError(this) << "Could not open file with name: " << m_file->fileName();
            return;
        }
    }
    QTextStream out(m_file);
    for (const auto &item : m_array) {
        out << QJsonDocument(item.toObject()).toJson(m_settings.format);
        Qt::endl(out);
    }
    m_array = QJsonArray();
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

void LoggingWorker::enqueueMsg(const WorkerMsg &msg)
{
    auto copy = msg;
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
    m_array.append(copy.toJsonObj());
    m_shouldUpdate = true;
}

void LoggingWorker::onMsg(const Radapter::WorkerMsg &msg)
{
    if (isFull() && m_settings.cycle_buffer) {
        if (testMsgForLog(msg)) {
            m_array.removeFirst();
            enqueueMsg(msg);
        }
    } else if (!isFull()) {
        if (testMsgForLog(msg)) {
            enqueueMsg(msg);
        }
    } else {
        workerError(this) << "File is Full";
    }
}
