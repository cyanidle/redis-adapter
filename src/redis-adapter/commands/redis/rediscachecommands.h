#ifndef REDIS_CACHECOMMANDS_H
#define REDIS_CACHECOMMANDS_H

#include <radapter-broker/command.h>
#include "redis-adapter/replies/redis/redisreplies.h"
namespace Redis {

class RADAPTER_SHARED_SRC ReadIndex : public Radapter::Command
{
    Q_GADGET
public:
    ReadIndex(const QString &index);
    QString index() const {return m_index;}
    GADGET_BODY
private:
    QString m_index;
};
class RADAPTER_SHARED_SRC ReadKeys : public Radapter::Command
{
    Q_GADGET
public:
    ReadKeys(const QStringList &keys);
    const QStringList &keys() const {return m_keys;}
    GADGET_BODY
private:
    QStringList m_keys;
};
class RADAPTER_SHARED_SRC ReadSet : public Radapter::Command
{
    Q_GADGET
public:
    ReadSet(const QString &set);
    const QString &set() const {return m_set;};
    using Reply = IndexReply;
    GADGET_BODY
private:
    QString m_set;
};

} // namespace Redis

RADAPTER_DECLARE_COMMAND(Redis::ReadIndex)
RADAPTER_DECLARE_COMMAND(Redis::ReadSet)
RADAPTER_DECLARE_COMMAND(Redis::ReadKeys)

#endif // REDIS_CACHECOMMANDS_H
