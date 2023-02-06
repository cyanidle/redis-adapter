#include "timeformatter.h"
#include "localization.h"
#include "radapterlogging.h"

#define MILLISECONDS_FORMAT "yyyy-MM-ddThh:mm:ss.zzz"

TimeFormatter::TimeFormatter(const QString &timeString)
{
    m_dateTime = QDateTime::fromString(timeString, MILLISECONDS_FORMAT);
    if (!m_dateTime.isValid()) {
        reDebug() << "TimeFormatter received invalid time string:" << timeString;
    }
}

TimeFormatter::TimeFormatter(const qint64 &timestamp)
{
    m_dateTime = QDateTime::fromMSecsSinceEpoch(timestamp, Localization::instance()->timeZone());
}

QDateTime TimeFormatter::dateTime() const
{
    return m_dateTime;
}

QString TimeFormatter::toMSecsString() const
{
    auto timeString = m_dateTime.toString(MILLISECONDS_FORMAT);
    return timeString;
}

QDateTime TimeFormatter::now() const
{
    auto currentTime = QDateTime::currentDateTime();
    return currentTime;
}
