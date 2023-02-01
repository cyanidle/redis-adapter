#ifndef TIMEFORMATTER_H
#define TIMEFORMATTER_H

#include <QObject>
#include <QDateTime>
#include <QTimeZone>

class RADAPTER_SHARED_SRC TimeFormatter
{
public:
    TimeFormatter() = default;
    explicit TimeFormatter(const QString &timeString);
    explicit TimeFormatter(const qint64 &timestamp);

    QDateTime dateTime() const;
    QString toMSecsString() const;
    QDateTime now() const;
private:
    QDateTime m_dateTime{};
};

#endif // TIMEFORMATTER_H
