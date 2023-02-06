#include "rediscommands.h"

namespace Redis {
namespace Cache {
ReadObject::ReadObject(const QString &index) :
    Radapter::Command(typeInConstructor(this)),
    m_key(index)
{

}

ReadKeys::ReadKeys(const QStringList &keys) :
    Radapter::Command(typeInConstructor(this)),
    m_keys(keys)
{

}

ReadSet::ReadSet(const QString &set) :
    Radapter::Command(typeInConstructor(this)),
    m_set(set)
{

}

ReadHash::ReadHash(const QString &hash) :
    Radapter::Command(typeInConstructor(this)),
    m_hash(hash)
{

}

ReadKey::ReadKey(const QString &key) :
    Radapter::Command(typeInConstructor(this)),
    m_key(key)
{

}

WriteObject::WriteObject(const QString &hashKey, const JsonDict &object) :
    Radapter::Command(typeInConstructor(this)),
    m_hashKey(hashKey),
    m_object(object)
{
}

Delete::Delete(const QString &target) :
    Radapter::Command(typeInConstructor(this)),
    m_target(target)
{
}

QString Delete::toRedisCommand() const
{
    return QStringLiteral("DELETE ").append(target());
}

WriteSet::WriteSet(const QString &set, const QStringList &keys) :
    Radapter::Command(typeInConstructor(this)),
    m_set(set),
    m_keys(keys)
{

}

QString WriteSet::toRedisCommand() const
{
    return QStringLiteral("SADD %1 %2").arg(set(), m_keys.join(" "));
}

WriteHash::WriteHash(const QString &hash, const QVariantMap &flatMap) :
    Radapter::Command(typeInConstructor(this)),
    m_hash(hash),
    m_flatMap(flatMap)
{

}

QString WriteHash::toRedisCommand() const
{
    auto command = QStringLiteral("HMSET ").append(hash());
    for (auto iter = m_flatMap.constBegin(); iter != m_flatMap.constEnd(); ++iter) {
        command.append(QStringLiteral(" %1 %2").arg(iter.key(), iter.value().toString()));
    }
    return command;
}

WriteKeys::WriteKeys(const QVariantMap &keys) :
    Radapter::Command(typeInConstructor(this)),
    m_keys(keys)
{

}

QString WriteKeys::toRedisCommand() const
{
    auto command = QStringLiteral("MSET");
    for (auto iter = m_keys.constBegin(); iter != m_keys.constEnd(); ++iter) {
        command.append(QStringLiteral(" %1 %2").arg(iter.key(), iter.value().toString()));
    }
    return command;
}

} // namespace Cache
} // namespace Redis
