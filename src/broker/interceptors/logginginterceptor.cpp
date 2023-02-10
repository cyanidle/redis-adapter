#include "logginginterceptor.h"
#include "radapterlogging.h"
#include <QDir>
#include "broker/worker/worker.h"
#include <QDateTime>

using namespace Radapter;

LoggingInterceptor::LoggingInterceptor(const LoggingInterceptorSettings &settings) :
    Radapter::InterceptorBase(),
    m_file(new QFile(settings.filepath, this)),
    m_flushTimer(new QTimer(this)),
    m_settings(settings),
    m_array(),
    m_error(false)
{
    connect(m_flushTimer, &QTimer::timeout, this, &LoggingInterceptor::onFlush);
    QDir().mkpath(settings.filepath.left(settings.filepath.lastIndexOf("/")));
    m_file->open(QIODevice::ReadOnly | QIODevice::Text);
    QJsonParseError err;
    QTextStream in(m_file);
    const QJsonDocument inDoc = QJsonDocument::fromJson(in.readAll().toUtf8(), &err);
    if (err.error != QJsonParseError::NoError && m_file->exists() && m_file->size()) {
        throw std::runtime_error(std::string("Will not overwrite contents of file: ") + baseFilepath().toStdString());
    }
    m_array = inDoc.array();
    m_file->close();
    m_flushTimer->start(settings.flush_delay);
}

bool LoggingInterceptor::isFull() const
{
    return static_cast<quint64>(m_file->size()) > m_settings.max_size_bytes;
}

void LoggingInterceptor::onFlush()
{
    if (m_error) {
        brokerError() << "Logger flush is impossible!";
    }
    if (!m_shouldUpdate) {
        return;
    }
    m_shouldUpdate = false;
    if (!m_file->isOpen()) {
        if (!m_file->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            brokerError() << "Could not open file with name: " << m_file->fileName();
            m_error = true;
            return;
        }
    }
    QTextStream out(m_file);
    out << QJsonDocument(m_array).toJson(m_settings.format);
    m_file->close();
}

bool LoggingInterceptor::testMsgForLog(const Radapter::WorkerMsg &msg) {
    if (m_settings.log.testFlag(LoggingInterceptorSettings::LogAll)) {
        return true;
    }
    if (!msg.isCommand() && !msg.isReply())
        return m_settings.log.testFlag(
            LoggingInterceptorSettings::LogNormal);
    if (msg.isReply())
        return m_settings.log.testFlag(
            LoggingInterceptorSettings::LogReply);
    if (msg.isCommand())
        return m_settings.log.testFlag(
            LoggingInterceptorSettings::LogCommand);
    return true;
}

void LoggingInterceptor::enqueueMsg(const WorkerMsg &msg)
{
    auto copy = msg;
    JsonDict metaInfo{QVariantMap{
        {"sender", msg.sender()->printSelf()},
        {"receivers", msg.printReceivers()},
        {"flags", msg.printFlags()},
        {"command", msg.command() ? msg.command()->metaObject()->className() : "None"},
        {"reply", msg.reply() ? msg.reply()->metaObject()->className() : "None"},
        {"serviceInfo", msg.printServiceData().toVariant()},
        {"id", msg.id()}
    }};
    copy["__meta__"] = metaInfo.toVariant();
    m_array.append(copy.toJsonObj());
    m_shouldUpdate = true;
}

void LoggingInterceptor::onMsgFromWorker(const Radapter::WorkerMsg &msg)
{
    emit msgToBroker(msg);
    if (isFull() && m_settings.rotating) {
        if (testMsgForLog(msg)) {
            m_array.removeFirst();
            enqueueMsg(msg);
        }
    } else if (!isFull()) {
        if (testMsgForLog(msg)) {
            enqueueMsg(msg);
        }
    } else {
        brokerWarn() << this << "File is Full";
    }
}
