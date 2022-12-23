#include "websocketclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "redis-adapter/radapterlogging.h"
#include "websocketconstants.h"

using namespace Websocket;

#define RECONNECT_TIMEOUT_MS        5000
#define HEARTBEAT_PERIOD_MS         1000
#define HEARTBEAT_REPLY_TIMEOUT_MS  10000

Client::Client(const QString &serverHost, quint16 serverPort, const Radapter::WorkerSettings &settings, QThread *thread)
    : WorkerBase(settings, thread),
      m_serverHost(serverHost),
      m_sock(nullptr),
      m_reconnectTimeout(RECONNECT_TIMEOUT_MS),
      m_heartbeatCounter(1)
{
    m_serverPort = serverPort > 0u ? serverPort : DEFAULT_WS_PORT;
    m_serverUrl = QString("ws://%1:%2").arg(m_serverHost).arg(m_serverPort);
}

Client::~Client()
{
    doStop();
}

void Client::initTimers()
{
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(m_reconnectTimeout);
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->callOnTimeout(this, &Client::run);

    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(HEARTBEAT_PERIOD_MS);
    m_heartbeatTimer->setSingleShot(false);
    m_heartbeatTimer->callOnTimeout(this, &Client::doPing);

    m_connectionLostTimer = new QTimer(this);
    m_connectionLostTimer->setInterval(HEARTBEAT_REPLY_TIMEOUT_MS);
    m_connectionLostTimer->setSingleShot(true);
    m_connectionLostTimer->callOnTimeout(this, &Client::onConnectionLost);
}

void Client::init()
{
    m_sock = new QWebSocket{ "radapter", QWebSocketProtocol::VersionLatest, this };
    connect(m_sock, &QWebSocket::connected, this, &Client::onConnected);
    connect(m_sock, &QWebSocket::disconnected, this, &Client::onDisconnected);
    connect(m_sock, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &Client::onError);
    connect(m_sock, &QWebSocket::textMessageReceived, this, &Client::onSocketReceived);
    connect(m_sock, &QWebSocket::pong, this, &Client::onPong);

    initTimers();
}

void Client::start()
{
    init();
    run();
}

void Client::run()
{
    if (isRunning()) {
        doStop();
    }
    doRun();
}

bool Client::isRunning() const
{
    return m_sock->isValid();
}

void Client::doRun()
{
    wclDebug() << "Websocket client: connecting to" << m_serverUrl;
    m_sock->open(m_serverUrl);
}

void Client::doStop()
{
    m_sock->close();
}

void Client::onConnected()
{
    wclDebug() << "Websocket client: connected successfully!";
    stopReconnect();
    resetHeartbeatCounter();
    startHeartbeat();
}

void Client::onDisconnected()
{
    wclDebug() << "Websocket client: disconnected";
    stopHeartbeat();
    startReconnect();
}

void Client::onError()
{
    wclDebug() << "Websocket error occured:" << m_sock->error() << m_sock->errorString();
    if (!isRunning()) {
        startReconnect();
    }
}

void Client::doPing()
{
    m_sock->ping(QString::number(m_heartbeatCounter).toUtf8());
}

void Client::onPong(quint64 elapsedTime, const QByteArray &payload)
{
    wclDebug() << "Websocket server heartbeat:" << payload << QString("(%1ms)").arg(elapsedTime);
    if (payload.toInt() == m_heartbeatCounter) {
        m_heartbeatCounter++;
        m_connectionLostTimer->start();
    }
}

void Client::onSocketReceived(const QString &message)
{
    auto parseStatus = QJsonParseError{};
    auto jsonMessage = QJsonDocument::fromJson(message.toUtf8(), &parseStatus);
    if (jsonMessage.isNull()) {
        wclDebug() << "json reply error:" << parseStatus.error << parseStatus.errorString();
        return;
    }
    if (jsonMessage.isEmpty()) {
        wclDebug() << "empty json received";
        return;
    }

    wclDebug().noquote() << "new message:" << jsonMessage.toJson(QJsonDocument::Indented);
    auto messageData = Formatters::Dict{ jsonMessage.toVariant() };
    if (!messageData.isEmpty()) {
        emit sendMsg(prepareMsg(messageData));
    }
}

void Client::onConnectionLost()
{
    wclDebug() << "Websocket server heartbeat has been timed out. Connection lost.";
    m_sock->close(QWebSocketProtocol::CloseCodeNormal, "connection lost");
}

void Client::startReconnect()
{
    if (!m_reconnectTimer->isActive()) {
        m_reconnectTimer->start();
    }
}

void Client::stopReconnect()
{
    m_reconnectTimer->stop();
}

void Client::startHeartbeat()
{
    if (!m_heartbeatTimer->isActive()) {
        m_heartbeatTimer->start();
    }
    m_connectionLostTimer->start();
}

void Client::stopHeartbeat()
{
    m_heartbeatTimer->stop();
    m_connectionLostTimer->stop();
}

void Client::resetHeartbeatCounter()
{
    m_heartbeatCounter = 1;
}
