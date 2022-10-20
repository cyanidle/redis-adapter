#include "protocol.h"
#include "radapter-broker/brokerlogging.h"

using namespace Radapter;


QMutex Protocol::m_mutex;
Protocol* Protocol::m_instance = nullptr;
QMap<QString, const CommandBase*> Protocol::m_customCommands = {};
CommandAcknowledgeJson* Protocol::m_ack = new CommandAcknowledgeJson();
CommandRequestJson* Protocol::m_json = new CommandRequestJson();
CommandRequestKeys* Protocol::m_keys = new CommandRequestKeys();
CommandDummy* Protocol::m_dummy = new CommandDummy();

Protocol::Protocol()
    : QObject()
{
}

const CommandBase* Protocol::custom(const QString &name)
{
    if (m_customCommands.contains(name)) {
        return m_customCommands.value(name);
    } else {
        brokerWarn() << "No such custom handler with name: " << name;
    }
    return m_dummy;
}


WorkerMsg CommandDummy::send(WorkerBase* sender, const QVariant &src) const
{
    Q_UNUSED(src)
    brokerError() << "Attempt to send a Dummy Command! (Probably custom command was not found)";
    brokerError() << "^ by:" << sender->workerName();
    return WorkerMsg();
}

QVariant CommandDummy::receive(const WorkerMsg &msg) const
{
    brokerError() << "Attempt to receive with Dummy Command! (Probably custom command was not found)";
    brokerError() << "^ msg by:" << msg.sender << "; Receivers: " << msg.receivers;
    return {};
}

QList<WorkerMsg::SenderType> CommandRequestKeys::supports() const
{
    return {
        WorkerMsg::TypeRedisCacheConsumer,
        WorkerMsg::TypeModbusConnector
    };
}

QVariant CommandRequestKeys::receive(const WorkerMsg &msg) const
{
    if (msg["meta"] == "__requestKeys") {
        return msg["data"].toStringList();
    }
    return QStringList();
}

WorkerMsg CommandRequestKeys::send(WorkerBase* sender, const QVariant &src) const
{
    auto command = prepareCommand(sender);
    command["meta"] = "__requestKeys";
    command["data"] = src;
    if (src.toStringList().isEmpty()) {
        brokerWarn() << "RequestKeys: Empty StringList of keys requested!";
    }
    return command;
}

QList<WorkerMsg::SenderType> CommandAcknowledgeJson::supports() const
{
    return {
        WorkerMsg::TypeRedisStreamConsumer,
        WorkerMsg::TypeModbusConnector
    };
}
QVariant CommandAcknowledgeJson::receive(const WorkerMsg &msg) const
{
    if (msg["meta"].toString() == "__acknowledge") {
        return msg["data"].toMap();
    }
    return Formatters::Dict();
}

WorkerMsg CommandAcknowledgeJson::send(WorkerBase* sender, const QVariant &src) const
{
    auto command = prepareCommand(sender);
    command["meta"] = "__acknowledge";
    command["data"] = src;
    if (src.toMap().isEmpty()) {
        brokerWarn() << "AckJson: Empty json sent for acknowledge";
    }
    return command;
}

QList<WorkerMsg::SenderType> CommandRequestJson::supports() const
{
    return {
        WorkerMsg::TypeRedisCacheConsumer,
        WorkerMsg::TypeModbusConnector,
        WorkerMsg::TypeRedisStreamConsumer
    };
}

QVariant CommandRequestJson::receive(const WorkerMsg &msg) const
{
    if (msg["meta"].toString() == "__requestJson") {
        return true;
    }
    return false;
}
WorkerMsg CommandRequestJson::send(WorkerBase* sender, const QVariant &src) const
{
    Q_UNUSED(src)
    auto command = prepareCommand(sender);
    command["meta"] = "__requestJson";
    command["data"] = src;
    return command;
}
