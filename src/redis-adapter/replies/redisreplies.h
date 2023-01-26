#ifndef REDIS_REPLIES_H
#define REDIS_REPLIES_H

#include "radapter-broker/reply.h"

namespace Redis {
namespace Cache {
class ReplyKeys : public Radapter::Reply
{
    Q_GADGET
public:
    ReplyKeys(const QStringList &keys);
    RADAPTER_REPLY
    const QStringList &keys() const {return m_keys;}
private:
    QStringList m_keys;
};

class ReplySet : public Radapter::Reply
{
    Q_GADGET
public:
    ReplySet(const QStringList &keys);
    RADAPTER_REPLY
    const QStringList &keys() const {return m_keys;}
private:
    QStringList m_keys;
};
}  // namespace Cache

} // namespace Redis
RADAPTER_DECLARE_REPLY(Redis::Cache::ReplyKeys)
RADAPTER_DECLARE_REPLY(Redis::Cache::ReplySet)
#endif // REDIS_REPLIES_H
