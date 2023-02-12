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

WriteSet::WriteSet(const QString &set, const QStringList &keys) :
    Radapter::Command(typeInConstructor(this)),
    m_set(set),
    m_keys(keys)
{

}

WriteHash::WriteHash(const QString &hash, const QVariantMap &flatMap) :
    Radapter::Command(typeInConstructor(this)),
    m_hash(hash),
    m_flatMap(flatMap)
{

}


WriteKeys::WriteKeys(const QVariantMap &keys) :
    Radapter::Command(typeInConstructor(this)),
    m_keys(keys)
{

}


} // namespace Cache
} // namespace Redis
