#include "rediscommands.h"

namespace Redis {
namespace Cache {
ReadIndex::ReadIndex(const QString &index) :
    Radapter::Command(Radapter::CommandType<ReadIndex>()),
    m_index(index)
{

}

ReadKeys::ReadKeys(const QStringList &keys) :
    Radapter::Command(Radapter::CommandType<ReadKeys>()),
    m_keys(keys)
{

}

ReadSet::ReadSet(const QString &set) :
    Radapter::Command(Radapter::CommandType<ReadSet>()),
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
