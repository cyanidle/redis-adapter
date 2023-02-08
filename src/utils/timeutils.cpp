#include "timeutils.h"
#include "localization.h"

#define MILLISECONDS_FORMAT "yyyy-MM-ddThh:mm:ss.zzz"


timeval Utils::Time::msecsToTimeval(qint32 milliseconds)
{
    timeval time{};
    time.tv_sec = milliseconds / 1000;
    time.tv_usec = milliseconds % 1000;
    return time;
}

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
