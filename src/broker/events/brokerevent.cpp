#include "brokerevent.h"

namespace Radapter {

BrokerEvent::BrokerEvent(QObject *sender, quint32 id, qint16 status, qint16 type, const QVariant &data) :
    m_sender(sender),
    m_id(id),
    m_status(status),
    m_type(type),
    m_data(data)
{

}

BrokerEvent::BrokerEvent(QObject *sender) :
    m_sender(sender)
{

}

BrokerEvent::BrokerEvent(QObject *sender, quint32 id, qint16 status, qint16 type, QVariant &&data) :
    m_sender(sender),
    m_id(id),
    m_status(status),
    m_type(type),
    m_data(std::move(data))
{

}

} // namespace Radapter
