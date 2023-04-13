#include "workermsg.h"
#include <QMetaEnum>
#include "radapterlogging.h"
#include "../worker.h"
#include "broker/broker.h"
#include "broker/replies/private/reply.h"

using namespace Radapter;

std::atomic<quint64> WorkerMsg::m_currentMsgId{0u};

Radapter::WorkerMsg::WorkerMsg() :
    JsonDict(),
    m_sender(),
    m_id(newMsgId()),
    m_receivers(),
    m_serviceData()
{
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

void WorkerMsg::ignoreReply()
{
    if (command()) {
        command()->ignoreReply();
    }
}

bool WorkerMsg::replyIgnored() const
{
    if (command()) {
        return command()->replyIgnored();
    } else {
        return false;
    }
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
    auto result = QStringLiteral("\n### Msg Id: ");
    result.reserve(150);
    auto flatData = QStringLiteral("# Flat Msg: \n") + printFlatData() + "\n\n";
    return result.append(QString::number(id())).append(QStringLiteral(" Debug Info ###\n"))
           .append(QStringLiteral("# Sender: ")).append(senderWorker->printSelf()).append("\n")
           .append(QStringLiteral("# Targets: [")).append(printReceivers().join(", ")).append("]\n")
           .append(QStringLiteral("# Flags: ")).append(printFlags()).append("\n")
           .append(json().isEmpty() ? "\n" : flatData)
           .append(QStringLiteral("# Command: ")).append(command() ? command()->metaObject()->className() : "None").append("\n")
           .append(QStringLiteral("# Reply: ")).append(reply() ? reply()->metaObject()->className() : "None").append("\n\n")
           .append(QStringLiteral("### Msg Id: ")).append(QString::number(id())).append(QStringLiteral(" Debug End  ###"));
}


const QSet<Worker *> &Radapter::WorkerMsg::receivers() const
{
    return m_receivers;
}

QVariant &Radapter::WorkerMsg::serviceData(ServiceData key)
{
    return m_serviceData[key];
}

QVariant Radapter::WorkerMsg::serviceData(ServiceData key) const
{
    return m_serviceData.value(key);
}

QVariant &Radapter::WorkerMsg::userData()
{
    return serviceData(ServiceUserData);
}

QVariant Radapter::WorkerMsg::userData() const
{
    return serviceData(ServiceUserData);
}

bool Radapter::WorkerMsg::testFlags(Flags flags) const noexcept
{
    return m_flags & flags;
}

void Radapter::WorkerMsg::setFlag(MsgFlag flag, bool value) noexcept
{
    m_flags.setFlag(flag, value);
}

void Radapter::WorkerMsg::clearFlags() noexcept
{
    m_flags = Flags(MsgOk);
}

void Radapter::WorkerMsg::clearJson()
{
    m_dict.clear();
}

QSet<Worker *> &Radapter::WorkerMsg::receivers() noexcept
{
    return m_receivers;
}

Worker *Radapter::WorkerMsg::sender() const
{
    return m_sender;
}

const JsonDict &Radapter::WorkerMsg::json() const
{
    return *this;
}

JsonDict &Radapter::WorkerMsg::json() {
    return *this;
}

const Command *Radapter::WorkerMsg::command() const
{
    return m_serviceData[ServiceCommand].value<QSharedPointer<Command>>().data();
}

Command *Radapter::WorkerMsg::command()
{
    return m_serviceData[ServiceCommand].value<QSharedPointer<Command>>().data();
}

const Reply *Radapter::WorkerMsg::reply() const
{
    return m_serviceData[ServiceReply].value<QSharedPointer<Reply>>().data();
}

Reply *Radapter::WorkerMsg::reply()
{
    return m_serviceData[ServiceReply].value<QSharedPointer<Reply>>().data();
}

QVariant &Radapter::WorkerMsg::privateData()
{
    return m_serviceData[ServicePrivate];
}

QVariant Radapter::WorkerMsg::privateData() const
{
    return m_serviceData[ServicePrivate];
}

void Radapter::WorkerMsg::updateId()
{
    m_id = newMsgId();
}

quint64 Radapter::WorkerMsg::newMsgId()
{
    return m_currentMsgId.fetch_add(1, std::memory_order_relaxed);
}

QDebug operator<<(QDebug dbg, const WorkerMsg &msg){
    dbg.nospace().noquote() << msg.printFullDebug();
    return dbg.maybeQuote().maybeSpace();
}
