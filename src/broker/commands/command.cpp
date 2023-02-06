#include "command.h"

namespace Radapter {

Command::Command(quint32 type) :
    m_type(type)
{

}

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


} // namespace Radapter
