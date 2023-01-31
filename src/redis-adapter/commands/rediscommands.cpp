#include "rediscommands.h"

namespace Redis {
namespace Cache {
ReadIndex::ReadIndex(const QString &index) :
    Radapter::Command(typeInConstructor(this)),
    m_index(index)
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

} // namespace Cache
} // namespace Redis
