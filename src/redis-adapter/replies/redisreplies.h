#ifndef REDIS_REPLIES_H
#define REDIS_REPLIES_H

#include "radapter-broker/reply.h"
#include "radapter-broker/basicreplies.h"

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
class ReplyKey : public Radapter::Reply
{
    Q_GADGET
public:
    ReplyKey(const QString &key);
    RADAPTER_REPLY
    const QString &key() const {return m_key;}
private:
    QString m_key;
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

class ReplyHash : public Radapter::Reply
{
    Q_GADGET
public:
    ReplyHash(const QVariantMap &flatHash);
    RADAPTER_REPLY
    const QVariantMap &flatHash() const {return m_flatHash;}
private:
    QVariantMap m_flatHash;
};
}  // namespace Cache

} // namespace Redis
RADAPTER_DECLARE_REPLY(Redis::Cache::ReplyKeys)
RADAPTER_DECLARE_REPLY(Redis::Cache::ReplyKey)
RADAPTER_DECLARE_REPLY(Redis::Cache::ReplySet)
RADAPTER_DECLARE_REPLY(Redis::Cache::ReplyHash)
#endif // REDIS_REPLIES_H
