#ifndef REDISKEYEVENTFORMATTER_H
#define REDISKEYEVENTFORMATTER_H

#include <QObject>
#include "jsondict/jsondict.h"

class RedisKeyEventFormatter : public QObject
{
    Q_OBJECT
public:
    explicit RedisKeyEventFormatter(const QVariantList &message, QObject *parent = nullptr);

    QString eventType() const;
    QString eventKey() const;
    JsonDict toEventMessage() const;

signals:

private:
    QVariantList m_message;
};

#endif // REDISKEYEVENTFORMATTER_H
