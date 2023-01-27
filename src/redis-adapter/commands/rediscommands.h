#ifndef REDIS_CACHECOMMANDS_H
#define REDIS_CACHECOMMANDS_H

#include "radapter-broker/basiccommands.h"
#include "redis-adapter/replies/redisreplies.h"
namespace Redis {
namespace Cache{
class RADAPTER_SHARED_SRC ReadIndex : public Radapter::Command
{
    Q_GADGET
public:
    ReadIndex(const QString &index);
    QString index() const {return m_index;}
    RADAPTER_COMMAND_WANTS(Radapter::ReplyWithJsonOk)
private:
    QString m_index;
};
class RADAPTER_SHARED_SRC ReadKeys : public Radapter::Command
{
    Q_GADGET
public:
    ReadKeys(const QStringList &keys);
    const QStringList &keys() const {return m_keys;}
    RADAPTER_COMMAND_WANTS(Redis::Cache::ReplyKeys)
private:
    QStringList m_keys;
};
class RADAPTER_SHARED_SRC ReadSet : public Radapter::Command
{
    Q_GADGET
public:
    ReadSet(const QString &set);
    const QString &set() const {return m_set;};
    RADAPTER_COMMAND_WANTS(Redis::Cache::ReplySet)
private:
    QString m_set;
};
} // namespace Cache
} // namespace Redis

RADAPTER_DECLARE_COMMAND(Redis::Cache::ReadIndex)
RADAPTER_DECLARE_COMMAND(Redis::Cache::ReadSet)
RADAPTER_DECLARE_COMMAND(Redis::Cache::ReadKeys)

#endif // REDIS_CACHECOMMANDS_H
