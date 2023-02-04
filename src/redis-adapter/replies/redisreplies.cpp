#include "redisreplies.h"

namespace Redis {
namespace Cache {

ReplyKeys::ReplyKeys(const QStringList &keys) :
    Reply(typeInConstructor(this), true),
    m_keys(keys)
{

}

ReplySet::ReplySet(const QStringList &keys) :
    Reply(typeInConstructor(this), true),
    m_keys(keys)
{

}

ReplyHash::ReplyHash(const QVariantMap &flatHash) :
    Reply(typeInConstructor(this), true),
    m_flatHash(flatHash)
{

}

ReplyKey::ReplyKey(const QString &key) :
    Reply(typeInConstructor(this), true),
    m_key(key)
{

}

} // namespace Cache
} // namespace Redis
