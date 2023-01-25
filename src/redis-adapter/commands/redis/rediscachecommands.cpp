#include "rediscachecommands.h"

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

} // namespace Cache
} // namespace Redis
