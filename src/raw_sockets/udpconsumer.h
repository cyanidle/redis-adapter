#ifndef UDP_CONSUMER_H
#define UDP_CONSUMER_H

#include "broker/worker/worker.h"
#include "settings-parsing/serializablesettings.h"
#include "settings/settings.h"
#include <QUdpSocket>
#include <QObject>

namespace Udp {

struct ConsumerSettings : Settings::SerializableSettings
{
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Settings::Required<Radapter::WorkerSettings>, worker)
    FIELD(Settings::Required<quint16>, port)
};

class Consumer : public Radapter::Worker
{
    Q_OBJECT
public:
    Consumer(const ConsumerSettings &settings, QThread *thread);
private slots:
    void readReady();
    void onError(QAbstractSocket::SocketError error);
    void onConnected();
    void onDisconnected();
private:
    QUdpSocket *m_socket;
};

} // namespace Udp

#endif // UDP_CONSUMER_H
