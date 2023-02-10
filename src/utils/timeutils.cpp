#include "timeutils.h"
#include "localization.h"

#define MILLISECONDS_FORMAT "yyyy-MM-ddThh:mm:ss.zzz"

QDateTime Utils::Time::fromString(const QString &string)
{
    return QDateTime::fromString(string, MILLISECONDS_FORMAT);
}

QDateTime Utils::Time::fromTimestamp(quint64 timestamp)
{
    return QDateTime::fromMSecsSinceEpoch(timestamp, Localization::instance()->timeZone());
}

QString Utils::Time::toMsecString(const QDateTime &datetime)
{
    return datetime.toString(MILLISECONDS_FORMAT);
}
