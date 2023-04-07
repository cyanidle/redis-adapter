#include "rediscachequeries.h"
#include "jsondict/jsondict.h"
#include <QStringList>

QString Redis::subscribeTo(const QStringList &events)
{
    return QStringLiteral("PSUBSCRIBE ") + events.join(' ');
}

QString Redis::toMultipleSet(const JsonDict &data)
{
    QStringList keys;
    for (auto &iter : data) {
        keys.append(iter.key().join(':') + iter.value().toString());
    }
    return QStringLiteral("MSET ") + keys.join(' ');
}

QString Redis::toUpdateSet(const QString &set, const QStringList &keys)
{
    return QStringLiteral("SADD %1 %2").arg(set, keys.join(' '));
}
