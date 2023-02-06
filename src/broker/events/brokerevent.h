#ifndef RADAPTER_WORKEREVENT_H
#define RADAPTER_WORKEREVENT_H

#include <QVariant>
#include <QObject>

namespace Radapter {
class Worker;
class Broker;
class BrokerEvent
{
public:
    BrokerEvent() = default;
    BrokerEvent(QObject *sender);
    BrokerEvent(QObject *sender, quint32 id, qint16 status = {}, qint16 type = {}, const QVariant &data = {});
    BrokerEvent(QObject *sender, quint32 id, qint16 status, qint16 type, QVariant &&data);
    QObject *sender() const {return m_sender;}
    const quint32 &id() const {return m_id;}
    const qint16 &status() const {return m_status;}
    quint32 &id() {return m_id;}
    qint16 &status() {return m_status;}
    const QVariant &data() const {return m_data;}
    template <class T>
    bool isFrom() const {
        return qobject_cast<T*>(m_sender) != nullptr;
    }
private:
    QObject *m_sender{};
    quint32 m_id{};
    qint16 m_status{};
    qint16 m_type{};
    QVariant m_data{};
};

} // namespace Radapter

Q_DECLARE_METATYPE(Radapter::BrokerEvent)
Q_DECLARE_TYPEINFO(Radapter::BrokerEvent, Q_MOVABLE_TYPE);

#endif // RADAPTER_WORKEREVENT_H
