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

} // namespace Cache
} // namespace Redis
