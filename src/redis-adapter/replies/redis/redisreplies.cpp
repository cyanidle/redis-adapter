#include "redisreplies.h"

namespace Redis {

IndexReply::IndexReply(const QStringList &keys) :
    Reply(Radapter::ReplyType<IndexReply>(), true),
    m_keys(keys)
{

}

} // namespace Redis
