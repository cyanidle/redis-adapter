#ifndef TIMEFORMATTER_H
#define TIMEFORMATTER_H

#include <QObject>
#include <QDateTime>
#include <QTimeZone>

class RADAPTER_SHARED_SRC TimeFormatter : public QObject
{
    Q_OBJECT
public:
    explicit TimeFormatter(QObject *parent = nullptr);
    explicit TimeFormatter(const QString &timeString, QObject *parent = nullptr);
    explicit TimeFormatter(const qint64 &timestamp,
                           const QTimeZone &timeZone,
                           QObject *parent = nullptr);
    explicit TimeFormatter(const qint64 &timestamp,
                           const QString &timeZoneString = QString{},
                           QObject *parent = nullptr);

    QDateTime dateTime() const;
    QString toMSecsString() const;
    QDateTime now() const;

signals:

private:
    QDateTime m_dateTime;
};

#endif // TIMEFORMATTER_H
