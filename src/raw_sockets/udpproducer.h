#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H

#include "broker/worker/worker.h"
#include "settings/settings.h"
#include <QUdpSocket>

namespace Udp {

struct ProducerSettings : Settings::SerializableSettings
{
    Q_GADGET
    IS_SERIALIZABLE
    SERIAL_FIELD(Radapter::WorkerSettings, worker)
    SERIAL_FIELD(Settings::ServerInfo, server)
};

class Producer: public Radapter::Worker
{
    Q_OBJECT
public:
    Producer(const ProducerSettings &settings, QThread *thread);
public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
private slots:
    void readReady();
    void onError(QAbstractSocket::SocketError error);
    void onConnected();
    void onDisconnected();
private:
    ProducerSettings m_settings;
    QUdpSocket *m_socket;
};

} // namespace Udp

#endif