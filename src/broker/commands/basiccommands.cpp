#include "basiccommands.h"
#include "radapterlogging.h"

namespace Radapter {

CommandAck::CommandAck(const JsonDict &json) :
    Command(typeInConstructor(this)),
    m_json(json)
{

}

CommandAck::CommandAck(JsonDict &&json) :
    Command(typeInConstructor(this)),
    m_json(std::move(json))
{

}

CommandTriggerRead::CommandTriggerRead() :
    Command(typeInConstructor(this))
{

}

CommandDummy::CommandDummy() :
    Command(typeInConstructor(this))
{

}

CommandPack::CommandPack(const QList<QSharedPointer<Command>> &commands) :
    Command(typeInConstructor(this)),
    m_commands(commands)
{
    checkPack();
}

CommandPack::CommandPack() :
    Command(typeInConstructor(this))
{
}

CommandPack::CommandPack(std::initializer_list<Command *> commands) :
    Command(typeInConstructor(this))
{
    for(auto &command : commands) {
        m_commands.append(QSharedPointer<Command>{command});
    }
    checkPack();
}

CommandPack::CommandPack(QList<QSharedPointer<Command>> &&commands) :
    Command(typeInConstructor(this)),
    m_commands(std::move(commands))
{
    checkPack();
}

int CommandPack::size() const
{
    return m_commands.size();
}

bool CommandPack::replyOk(const Reply *reply) const
{
    return Command::replyOk(reply) &&
           reply->as<ReplyPack>()->replies().count() == m_commands.count();
}

void CommandPack::append(Command *newCommand)
{
    m_commands.append(QSharedPointer<Command>(newCommand));
}

bool CommandPack::isEmpty() const
{
    return m_commands.isEmpty();
}

const Command *CommandPack::first() const
{
    return m_commands.first().data();
}

const QList<QSharedPointer<Command>> &CommandPack::commands() const
{
    return m_commands;
}

QList<QSharedPointer<Command>> &CommandPack::commands()
{
    return m_commands;
}

void CommandPack::checkPack() const
{
    for (auto &command : m_commands) {
        if (command->inherits<CommandPack>()) throw std::invalid_argument("Command Pack cannot contain Command Packs!");
    }
}

CommandTrigger::CommandTrigger() :
    Command(typeInConstructor(this))
{

}

} // namespace Radapter
