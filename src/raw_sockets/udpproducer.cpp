#include "udpproducer.h"
#include "broker/workers/private/workermsg.h"
#include <QNetworkDatagram>
using namespace Radapter;
using namespace Udp;

Producer::Producer(const ProducerSettings &settings, QThread *thread) :
    Radapter::Worker(settings.worker, thread),
    m_settings(settings)
{
    setRole(Worker::Consumer);
    m_socket = new QUdpSocket(this);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &Producer::onError);
    connect(m_socket, &QAbstractSocket::connected, this, &Producer::onConnected);
    connect(m_socket, &QAbstractSocket::disconnected, this, &Producer::onDisconnected);
    connect(m_socket, &QAbstractSocket::readyRead, this, &Producer::readReady);
}

void Producer::onMsg(const Radapter::WorkerMsg &msg)
{
    static QByteArray crlf("\r\n");
    auto payload = msg.json().toBytes().append(crlf);
    auto datagram = QNetworkDatagram(payload, QHostAddress(m_settings.server->host.value), m_settings.server->port);
    m_socket->writeDatagram(datagram);
}

void Producer::readReady()
{
    workerError(this) << "Write into producer!";
}

void Producer::onError(QAbstractSocket::SocketError error)
{
    workerError(this) << ": Error: " << QMetaEnum::fromType<QAbstractSocket::SocketError>().valueToKey(error);
}

void Producer::onConnected()
{
    workerInfo(this) << "Connected!";
}

void Producer::onDisconnected()
{
    workerWarn(this) << "Disconnected!";
}
