#include "redisreplies.h"

namespace Redis {
namespace Cache {

ReplyKeys::ReplyKeys(const QStringList &keys) :
    Reply(Radapter::ReplyType<ReplyKeys>(), true),
    m_keys(keys)
{

}

ReplySet::ReplySet(const QStringList &keys) :
    Reply(Radapter::ReplyType<ReplySet>(), true),
    m_keys(keys)
{

}

} // namespace Cache
} // namespace Redis
