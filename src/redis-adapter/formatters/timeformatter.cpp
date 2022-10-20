#include "timeformatter.h"
#include "redis-adapter/localization.h"
#include "redis-adapter/radapterlogging.h"

#define MILLISECONDS_FORMAT "yyyy-MM-ddThh:mm:ss.zzz"

TimeFormatter::TimeFormatter(QObject *parent)
    : QObject(parent)
{
}

TimeFormatter::TimeFormatter(const QString &timeString, QObject *parent)
    : QObject(parent)
{
    m_dateTime = QDateTime::fromString(timeString, MILLISECONDS_FORMAT);
    if (!m_dateTime.isValid()) {
        reDebug() << "TimeFormatter received invalid time string:" << timeString;
    }
}

TimeFormatter::TimeFormatter(const qint64 &timestamp, const QTimeZone &timeZone, QObject *parent)
    : QObject(parent)
{
    m_dateTime = QDateTime::fromMSecsSinceEpoch(timestamp, timeZone);
}

TimeFormatter::TimeFormatter(const qint64 &timestamp, const QString &timeZoneString, QObject *parent)
    : QObject(parent)
{
    auto timeZone = QTimeZone(timeZoneString.toLatin1());
    if (!timeZone.isValid()) {
        timeZone = Localization::instance()->timeZone();
    }
    m_dateTime = QDateTime::fromMSecsSinceEpoch(timestamp, timeZone);
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
