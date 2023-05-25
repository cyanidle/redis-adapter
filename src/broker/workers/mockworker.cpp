#include "mockworker.h"
#include <QJsonArray>
#include <QJsonObject>
#include "broker/workers/private/workermsg.h"
#include "private/global.h"
#include "radapterlogging.h"
#include <QTimer>
#include "jsondict/jsondict.h"
#include <QFile>
#include "settings/mockworkersettings.h"

using namespace Radapter;

struct MockWorker::Private {
    QFile* file;
    QList<JsonDict> jsons{};
    QTimer* mockTimer{};
    int currentIndex{};
};

MockWorker::MockWorker(const Settings::MockWorker &settings, QThread* thread) :
    Worker(settings, thread),
    d(new Private{
        new QFile(settings.json_file_path)
    })
{
    workerConfig().print_msgs = true;
    if (settings.json_file_path->isEmpty()) {
        return;
    }
    d->mockTimer = new QTimer(this);
    d->mockTimer->setInterval(settings.mock_timer_delay);
    connect(d->mockTimer, &QTimer::timeout, this, &MockWorker::onMock);
    if (!d->file->isOpen()) {
        if (!d->file->open(QIODevice::ReadOnly)) {
            brokerError() << "Error opening Json file: " << d->file->fileName();
            throw std::runtime_error("Could not open Json File: " + d->file->fileName().toStdString());
        }
    }
    QTextStream in(d->file);
    QJsonParseError err;
    const QJsonDocument inputDoc = QJsonDocument::fromJson(in.readAll().toUtf8(), &err);
    const QJsonArray jsonArray = inputDoc.array();
    if (err.error != QJsonParseError::NoError) {
        brokerError() << "Error parsing Json in file: " << d->file->fileName();
        brokerError() << "Full reason: " << err.errorString();
    }
    for (const auto &item : jsonArray) {
        d->jsons.append(JsonDict::fromJsonObj(item.toObject()));
    }
    d->file->close();
}

MockWorker::~MockWorker()
{
    delete d;
}

void MockWorker::onRun()
{
    if (d->mockTimer) {
        d->mockTimer->start();
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
    if (d->jsons.isEmpty()) {
        throw std::runtime_error("MockWorker --> Empty Jsons List");
    }
    if (d->currentIndex >= d->jsons.size()) {
        d->currentIndex = 0;
    }
    return d->jsons.at(d->currentIndex++);
}

void MockWorker::onMock()
{
    if (d->jsons.isEmpty()) {
        return;
    }
    auto currentMsg = getNext();
    auto mockCommands = currentMsg.take("__mock_commands__").toMap();
    if (mockCommands.contains("ignore") && mockCommands.value("ignore").toBool()) {
        onMock();
        return;
    }
    if (mockCommands.contains("once") && mockCommands.value("once").toBool()) {
        d->jsons.removeAt(d->currentIndex);
    }
    /// \todo Add commands for mock behaviour
    const auto msg = prepareMsg(currentMsg);
    emit sendMsg(msg);
}

