#include "command.h"
#include <stdexcept>
namespace Radapter {

Command::Command(quint32 type) :
    m_type(type),
    m_ignoreReply(false)
{
}

bool Command::isUser() const {return User < type();}

Command::Type Command::type() const {return static_cast<Type>(m_type);}

const QMetaObject *Command::metaObject() const {return &this->staticMetaObject;}

bool Command::isWantedReply(const Reply *reply) const
{
    return reply &&
           (wantedReplyType() == reply->type() ||
            reply->metaObject()->inherits(wantedReplyMetaObject()));
}

bool Command::replyOk(const Reply *reply) const {
    return isWantedReply(reply) && reply->ok();
}

quint32 Command::generateUserType()
{
    static std::atomic<quint32> userCount{0u};
    auto next = userCount.fetch_add(1, std::memory_order_relaxed) + Command::User;
    if (next == Command::MaxUser) {
        throw std::invalid_argument("Max User Types Reached");
    }
    return next;
}

void *Command::voidCast(const QMetaObject *meta)
{
    return meta && metaObject()->inherits(meta) ? this : nullptr;
}

const void *Command::voidCast(const QMetaObject *meta) const
{
    return const_cast<Command*>(this)->voidCast(meta);
}

void Command::ignoreReply()
{
    m_ignoreReply = true;
}

bool Command::replyIgnored() const
{
    return m_ignoreReply;
}

void Command::setCallback(const CommandCallback &cb)
{
    m_cb = cb;
}

void Command::setFailCallback(const CommandCallback &cb)
{
    m_failCb = cb;
}

CommandCallback &Command::callback()
{
    return m_cb;
}

const CommandCallback &Command::failCallback() const
{
    return m_failCb;
}

CommandCallback &Command::failCallback()
{
    return m_failCb;
}

const CommandCallback &Command::callback() const
{
    return m_cb;
}


} // namespace Radapter
