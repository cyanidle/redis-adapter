#include "rediscachecommands.h"

namespace Redis {

ReadIndex::ReadIndex(const QString &index) :
    Radapter::Command(Type()),
    m_index(index)
{

}

ReadKeys::ReadKeys(const QStringList &keys) :
    Radapter::Command(Type()),
    m_keys(keys)
{

}

ReadSet::ReadSet(const QString &set) :
    Radapter::Command(Type()),
    m_set(set)
{

}


} // namespace Redis
