#include "workermsg.h"
#include "radapterlogging.h"
#include "../worker.h"
#include "broker/broker.h"
#include "broker/commands/basiccommands.h"
#include "templates/algorithms.hpp"
#include "broker/replies/private/reply.h"

using namespace Radapter;

std::atomic<quint64> WorkerMsg::m_currentMsgId{0u};

Radapter::WorkerMsg::WorkerMsg() :
    JsonDict(),
    m_flags(MsgBad),
    m_sender(),
    m_id(newMsgId()),
    m_receivers(),
    m_serviceData()
{
    serviceData(ServiceBadReasonField) = "Sender not set";
}

Radapter::WorkerMsg::WorkerMsg(Worker *sender,
                               const QStringList &receivers) :
    JsonDict(),
    m_flags(MsgOk),
    m_sender(sender),
    m_id(newMsgId()),
    m_receivers(),
    m_serviceData()
{
    for(auto& name:receivers) {
        auto worker = broker()->getWorker(name);
        if (!worker) throw std::invalid_argument("Inexistent worker: " + name.toStdString());
        m_receivers.insert(worker);
    }
}

WorkerMsg::WorkerMsg(Worker *sender,
                     const QSet<Worker *> &receivers) :
    JsonDict(),
    m_flags(MsgOk),
    m_sender(sender),
    m_id(newMsgId()),
    m_receivers(receivers),
    m_serviceData()
{
}

Broker *WorkerMsg::broker()
{
    return Broker::instance();
}

QStringList WorkerMsg::printReceivers() const {
    QStringList result;
    for (auto &worker: m_receivers) {
        result.append(worker->printSelf());
    }
    return result;
}

QString WorkerMsg::printFlags() const {
    QString result;
    if (m_flags.testFlag(MsgBad))
        result += "|Bad";
    else
        result += "|Ok";
    if (m_flags.testFlag(MsgDirect))
        result += "|Direct";
    if (m_flags.testFlag(MsgBroadcast))
        result += "|Broadcast";
    if (isCommand())
        result += "|Command";
    if (isReply())
        result += "|Reply";
    return result;
}

JsonDict WorkerMsg::printServiceData() const
{
    JsonDict result;
    auto metaEnum = QMetaEnum::fromType<ServiceData>();
    for (auto iter = m_serviceData.constBegin(); iter != m_serviceData.constEnd(); ++iter) {
        if (iter.key() == ServiceCommand) {
            continue;
        } else if (iter.key() == ServiceReply) {
            continue;
        }
        result.insert(metaEnum.valueToKey(iter.key()), iter.value());
    }
    return result;
}

QString WorkerMsg::printFlatData() const
{
    QStringList result;
    auto flat = flatten(":");
    auto biggestSize = 0;
    for (auto key = flat.keyBegin(); key != flat.keyEnd(); ++key) {
        auto sz = key->size();
        if (sz > biggestSize) {
            biggestSize = sz;
        }
    }
    for (auto keyVal = flat.constBegin(); keyVal != flat.constEnd(); ++keyVal) {
        auto toAppend = keyVal.key();
        toAppend.append(":");
        toAppend.resize(biggestSize + 3, ' ');
        toAppend.append(keyVal.value().value<QString>());
        result.append(toAppend);
    }
    return "\n# " + result.join("\n# ");
}

QString WorkerMsg::printFullDebug() const
{
    auto senderWorker = sender();
    if (!senderWorker) {
        brokerError() << "Msg ID: " << m_id << " --> sender is not a workerBase!";
        return "Sender Error!";
    }
    auto result = QStringLiteral("\n ### Msg Id: ");
    result.reserve(100);
    return result + QString::number(id()) + QStringLiteral(" Debug Info ###\n") +
           QStringLiteral("# Sender: ") + senderWorker->printSelf()  + "\n" +
           QStringLiteral("# Targets: [") + printReceivers().join(", ") + "]\n" +
           QStringLiteral("# Flags: ") + printFlags() + "\n" +
           QStringLiteral("# Flat Msg: \n") + printFlatData() + "\n\n" +
           QStringLiteral("# Command: ") + (command() ? command()->metaObject()->className() : "None") + "\n" +
           QStringLiteral("# Reply: ") + (reply() ? reply()->metaObject()->className() : "None") + "\n" +
           QStringLiteral("### Msg Id: ") + QString::number(id()) + QStringLiteral(" Debug End  ###");
}

