#include "reply.h"

namespace Radapter {

Reply::Reply(quint32 type, bool ok) :
    m_type(type),
    m_ok(ok)
{

}

quint32 Reply::generateUserType()
{
    static std::atomic<quint32> userCount{0u};
    auto next = userCount.fetch_add(1, std::memory_order_relaxed) + Reply::User;
    if (next == Reply::MaxUser) {
        throw std::invalid_argument("Max User Types Reached");
    }
    return next;
}

void *Reply::voidCast(const QMetaObject *meta)
{
    return meta && metaObject()->inherits(meta) ? this : nullptr;
}

const void *Reply::voidCast(const QMetaObject *meta) const
{
    return const_cast<Reply*>(this)->voidCast(meta);
}


} // namespace Radapter
