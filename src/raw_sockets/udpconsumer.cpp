#include "udpconsumer.h"
#include "jsondict/jsondict.h"
#include <QNetworkDatagram>
using namespace Udp;
using namespace Radapter;

Consumer::Consumer(const ConsumerSettings &settings, QThread *thread) :
    Radapter::Worker(settings.worker, thread)
{
    setRole(Worker::Producer);
    m_socket = new QUdpSocket(this);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &Consumer::onError);
    connect(m_socket, &QAbstractSocket::connected, this, &Consumer::onConnected);
    connect(m_socket, &QAbstractSocket::disconnected, this, &Consumer::onDisconnected);
    connect(m_socket, &QAbstractSocket::readyRead, this, &Consumer::readReady);
    auto bindOk = m_socket->bind(settings.port);
    if (!bindOk) {
        throw std::runtime_error("Could not bind to port: " + QString::number(settings.port).toStdString());
    }
}

void Consumer::readReady()
{
    while (m_socket->hasPendingDatagrams()) {
        auto info = m_socket->receiveDatagram();
        workerDebug(this) << "Receiving from: Host:" << info.senderAddress() << "Port:" << info.senderPort();
        QJsonParseError err;
        auto asJson = JsonDict::fromBytes(info.data(), &err);
        if (err.error != QJsonParseError::NoError) {
            workerError(this) << "Json Parse error!";
            return;
        }
        emit send(asJson);
    }
}

void Consumer::onError(QAbstractSocket::SocketError error)
{
    workerError(this) << ": Error: " << QMetaEnum::fromType<QAbstractSocket::SocketError>().valueToKey(error);
}

void Consumer::onConnected()
{
    workerInfo(this) << "Connected!";
}

void Consumer::onDisconnected()
{
    workerWarn(this) << "Disconnected!";
}
