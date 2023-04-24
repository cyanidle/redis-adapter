#include "mockworker.h"
#include <QJsonArray>
#include <QJsonObject>
#include "broker/workers/private/workermsg.h"
#include "private/global.h"
#include "radapterlogging.h"
#include "settings/mockworkersettings.h"

using namespace Radapter;

MockWorker::MockWorker(const Settings::MockWorker &settings, QThread* thread) :
    Worker(settings, thread),
    m_file(new QFile(settings.json_file_path)),
    m_mockTimer(),
    m_currentIndex()
{
    if (settings.json_file_path->isEmpty()) {
        return;
    }
    m_mockTimer = new QTimer(this);
    m_mockTimer->setInterval(settings.mock_timer_delay);
    connect(m_mockTimer, &QTimer::timeout, this, &MockWorker::onMock);
    if (!m_file->isOpen()) {
        if (!m_file->open(QIODevice::ReadOnly)) {
            brokerError() << "Error opening Json file: " << m_file->fileName();
            throw std::runtime_error("Could not open Json File: " + m_file->fileName().toStdString());
        }
    }
    QTextStream in(m_file);
    QJsonParseError err;
    const QJsonDocument inputDoc = QJsonDocument::fromJson(in.readAll().toUtf8(), &err);
    const QJsonArray jsonArray = inputDoc.array();
    if (err.error != QJsonParseError::NoError) {
        brokerError() << "Error parsing Json in file: " << m_file->fileName();
        brokerError() << "Full reason: " << err.errorString();
    }
    for (const auto &item : jsonArray) {
        m_jsons.append(JsonDict::fromJsonObj(item.toObject()));
    }
    m_file->close();
}

void MockWorker::onRun()
{
    if (m_mockTimer) {
        m_mockTimer->start();
    }
    Worker::onRun();
}

void MockWorker::onMsg(const Radapter::WorkerMsg &msg)
{
    brokerInfo().noquote() << "Mock worker received Msg! Id:" << msg.id();
}

void MockWorker::onReply(const Radapter::WorkerMsg &msg)
{
    brokerInfo().noquote() << "Mock worker received Reply! Id:" << msg.id();
}

void MockWorker::onCommand(const Radapter::WorkerMsg &msg)
{
    brokerInfo().noquote()  << "Mock worker received Command! Id:"<< msg.id();
}

const JsonDict &MockWorker::getNext()
{
    if (m_jsons.isEmpty()) {
        throw std::runtime_error("MockWorker --> Empty Jsons List");
    }
    if (m_currentIndex >= m_jsons.size()) {
        m_currentIndex = 0;
    }
    return m_jsons.at(m_currentIndex++);
}

void MockWorker::onMock()
{
    if (m_jsons.isEmpty()) {
        return;
    }
    auto currentMsg = getNext();
    auto mockCommands = currentMsg.take("__mock_commands__").toMap();
    if (mockCommands.contains("ignore") && mockCommands.value("ignore").toBool()) {
        onMock();
        return;
    }
    if (mockCommands.contains("once") && mockCommands.value("once").toBool()) {
        m_jsons.removeAt(m_currentIndex);
    }
    /// \todo Add commands for mock behaviour
    const auto msg = prepareMsg(currentMsg);
    emit sendMsg(msg);
}

