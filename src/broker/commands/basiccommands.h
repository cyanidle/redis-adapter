#ifndef RADAPTER_BASICCOMMANDS_H
#define RADAPTER_BASICCOMMANDS_H

#include "private/command.h"
#include "broker/replies/basicreplies.h"
#include "jsondict/jsondict.hpp"

namespace Radapter {

class RADAPTER_API CommandAck : public Command
{
    Q_GADGET
public:
    CommandAck(const JsonDict &json);
    CommandAck(JsonDict &&json);
    const JsonDict &json() const {return m_json;}
    RADAPTER_COMMAND_WANTS(Radapter::Reply)
private:
    JsonDict m_json;
};

class RADAPTER_API CommandTriggerRead : public Command
{
    Q_GADGET
public:
    CommandTriggerRead();
    RADAPTER_COMMAND_WANTS(Radapter::Reply)
};

class RADAPTER_API CommandTrigger : public Command
{
    Q_GADGET
public:
    CommandTrigger();
    RADAPTER_COMMAND_WANTS(Radapter::Reply)
};

class RADAPTER_API CommandDummy : public Command
{
    Q_GADGET
public:
    CommandDummy();
    RADAPTER_COMMAND_WANTS(Radapter::Reply)
};

class RADAPTER_API CommandPack : public Command
{
    Q_GADGET
public:
    CommandPack();
    CommandPack(std::initializer_list<Command *> commands);
    CommandPack(const QList<QSharedPointer<Command>> &commands);
    CommandPack(QList<QSharedPointer<Command>> &&commands);
    int size() const;
    virtual bool replyOk(const Radapter::Reply* reply) const override;
    void append(Command *newCommand);
    bool isEmpty() const;
    RADAPTER_COMMAND_WANTS(Radapter::ReplyPack)
    const Command *first() const;
    const QList<QSharedPointer<Command>> &commands() const;
    QList<QSharedPointer<Command>> &commands();
private:
    void checkPack() const;
    QList<QSharedPointer<Command>> m_commands{};
};
} // namespace Radapter
IMPL_RADAPTER_DECLARE_COMMAND_BUILTIN(Radapter::CommandAck, Command::Acknowledge)
IMPL_RADAPTER_DECLARE_COMMAND_BUILTIN(Radapter::CommandTriggerRead, Command::RequestJson)
IMPL_RADAPTER_DECLARE_COMMAND_BUILTIN(Radapter::CommandDummy, Command::Dummy)
IMPL_RADAPTER_DECLARE_COMMAND_BUILTIN(Radapter::CommandPack, Command::Pack)
IMPL_RADAPTER_DECLARE_COMMAND_BUILTIN(Radapter::CommandTrigger, Command::Trigger)
#endif // RADAPTER_BASICCOMMANDS_H
