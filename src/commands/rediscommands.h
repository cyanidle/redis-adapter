#ifndef REDIS_CACHECOMMANDS_H
#define REDIS_CACHECOMMANDS_H

#include "broker/commands/basiccommands.h"
#include "replies/redisreplies.h"
namespace Redis {
namespace Cache{
class RADAPTER_SHARED_SRC ReadObject : public Radapter::Command
{
    Q_GADGET
public:
    ReadObject(const QString &index);
    QString key() const {return m_key;}
    RADAPTER_COMMAND_WANTS(Radapter::ReplyJson)
private:
    QString m_key;
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
class RADAPTER_SHARED_SRC ReadKey : public Radapter::Command
{
    Q_GADGET
public:
    ReadKey(const QString &key);
    const QString &key() const {return m_key;}
    RADAPTER_COMMAND_WANTS(Redis::Cache::ReplyKey)
private:
    QString m_key;
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
class RADAPTER_SHARED_SRC ReadHash : public Radapter::Command
{
    Q_GADGET
public:
    ReadHash(const QString &hash);
    const QString &hash() const {return m_hash;};
    RADAPTER_COMMAND_WANTS(Redis::Cache::ReplyHash)
private:
    QString m_hash;
};

//! Producer commands
class RADAPTER_SHARED_SRC WriteHash : public Radapter::Command
{
    Q_GADGET
public:
    WriteHash(const QString &hash, const QVariantMap &flatMap);
    const QVariantMap &flatMap() const {return m_flatMap;}
    const QString &hash() const {return m_hash;};
    RADAPTER_COMMAND_WANTS(Radapter::Reply)
private:
    QString m_hash;
    QVariantMap m_flatMap;
};

class RADAPTER_SHARED_SRC WriteSet : public Radapter::Command
{
    Q_GADGET
public:
    WriteSet(const QString &set, const QStringList& keys);
    const QString &set() const {return m_set;};
    const QStringList &keys() const {return m_keys;}
    RADAPTER_COMMAND_WANTS(Radapter::Reply)
private:
    QString m_set;
    QStringList m_keys;
};

class RADAPTER_SHARED_SRC Delete : public Radapter::Command
{
    Q_GADGET
public:
    Delete(const QString &target);
    const QString &target() const {return m_target;};
    RADAPTER_COMMAND_WANTS(Radapter::Reply)
private:
    QString m_target;
};
class RADAPTER_SHARED_SRC WriteKeys : public Radapter::Command
{
    Q_GADGET
public:
    WriteKeys(const QVariantMap &keys);
    const QVariantMap &keys() const {return m_keys;}
    RADAPTER_COMMAND_WANTS(Radapter::Reply)
private:
    QVariantMap m_keys;
};
class RADAPTER_SHARED_SRC WriteObject : public Radapter::Command
{
    Q_GADGET
public:
    WriteObject(const QString &hashKey, const JsonDict &object);
    const JsonDict &object() const {return m_object;}
    QString hashKey() const {return m_hashKey;}
    RADAPTER_COMMAND_WANTS(Radapter::Reply)
private:
    QString m_hashKey;
    JsonDict m_object;
};
} // namespace Cache
} // namespace Redis

RADAPTER_DECLARE_COMMAND(Redis::Cache::ReadObject)
RADAPTER_DECLARE_COMMAND(Redis::Cache::ReadSet)
RADAPTER_DECLARE_COMMAND(Redis::Cache::ReadKeys)
RADAPTER_DECLARE_COMMAND(Redis::Cache::ReadKey)
RADAPTER_DECLARE_COMMAND(Redis::Cache::ReadHash)
RADAPTER_DECLARE_COMMAND(Redis::Cache::Delete)
RADAPTER_DECLARE_COMMAND(Redis::Cache::WriteObject)
RADAPTER_DECLARE_COMMAND(Redis::Cache::WriteHash)
RADAPTER_DECLARE_COMMAND(Redis::Cache::WriteKeys)
RADAPTER_DECLARE_COMMAND(Redis::Cache::WriteSet)

#endif // REDIS_CACHECOMMANDS_H
