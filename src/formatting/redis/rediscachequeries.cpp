#include "rediscachequeries.h"
#include "jsondict/jsondict.hpp"
#include <QStringList>

QString Redis::subscribeTo(const QStringList &events)
{
    return QStringLiteral("PSUBSCRIBE ") + events.join(' ');
}

QString Redis::multipleSet(const JsonDict &data)
{
    QStringList keys;
    for (auto &iter : data) {
        keys.append(iter.key().join(':') + iter.value().toString());
    }
    return QStringLiteral("MSET ") + keys.join(' ');
}

QString Redis::updateSet(const QString &set, const QStringList &keys)
{
    return QStringLiteral("SADD %1 %2").arg(set, keys.join(' '));
}
