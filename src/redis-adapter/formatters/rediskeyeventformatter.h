#ifndef REDISKEYEVENTFORMATTER_H
#define REDISKEYEVENTFORMATTER_H

#include <QObject>
#include "JsonFormatters"

class RedisKeyEventFormatter : public QObject
{
    Q_OBJECT
public:
    explicit RedisKeyEventFormatter(const Formatters::List &message, QObject *parent = nullptr);

    QString eventType() const;
    QString eventKey() const;
    Formatters::Dict toEventMessage() const;

signals:

private:
    Formatters::List m_message;
};

#endif // REDISKEYEVENTFORMATTER_H
