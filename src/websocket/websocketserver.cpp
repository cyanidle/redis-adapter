#include "websocketserver.h"
#include <QTimer>
#include <QJsonDocument>
#include <QWebSocketServer>
#include <QWebSocket>
#include "radapterlogging.h"

#define PING_PAYLOAD "PING"
#define PONG_PAYLOAD "PING"

using namespace Websocket;
using namespace Radapter;

QString printClient(QObject *client) {
    auto asWs = qobject_cast<QWebSocket*>(client);
    if (!asWs) return "Unknown";
    return QStringLiteral("[%1] %2:%3").arg(asWs->peerName(), asWs->peerAddress().toString(), QString::number(asWs->peerPort()));
}

Server::Server(const Settings::WebsocketServer &settings, QThread *thread) :
    Worker(settings.worker, thread),
    m_pingTimer(new QTimer(this)),
    m_websocketServer(new QWebSocketServer(settings.name,
                                           settings.secure ? QWebSocketServer::SecureMode : QWebSocketServer::NonSecureMode,
                                           this)),
    m_settings(settings),
    m_isConnected(false)
{
    m_pingTimer->setInterval(m_settings.heartbeat_ms);
    m_pingTimer->callOnTimeout(this, &Server::checkClients);
    if (settings.secure) {
        throw std::runtime_error("Secure Websockets are not supported yet!");
    }
    connect(m_websocketServer, &QWebSocketServer::newConnection, this, &Server::onNewConnection);
    connect(m_websocketServer, &QWebSocketServer::serverError, this, [this](QWebSocketProtocol::CloseCode code){
        workerError(this) << "Error from WSServer:" << code;
    });
    connect(m_websocketServer, &QWebSocketServer::acceptError, this, [this](QAbstractSocket::SocketError err){
        workerError(this) << "Accept Error from WSServer:" << QMetaEnum::fromType<decltype(err)>().valueToKey(err);
    });
}

bool Server::isConnected() const
{
    return m_isConnected;
}

Server::~Server()
{
    for (auto client: m_clients) {
        client->close(QWebSocketProtocol::CloseCodeNormal, "shutdown");
    }
}

void Server::checkClients()
{
    for (auto client: m_clients) {
        if (m_ttls[client] < 0) {
            workerWarn(this) << "Client:" << printClient(client) << " --> timeout";
            client->close(QWebSocketProtocol::CloseCodeAbnormalDisconnection, "Connection timeout");
        } else {
            client->ping(PING_PAYLOAD);
        }
        m_ttls[client] -= m_settings.heartbeat_ms;
    }
}

void Server::onNewConnection()
{
    auto socket = m_websocketServer->nextPendingConnection();
    socket->setParent(this);
    connect(socket, &QWebSocket::disconnected, this, [socket, this](){
        workerWarn(this) << "Client:" << printClient(socket) << " --> disconnected";
        m_ttls.remove(socket);
        m_clients.remove(socket);
        socket->deleteLater();
    });
    connect(socket, &QWebSocket::textMessageReceived, this, &Server::onTextMsg);
    connect(socket, &QWebSocket::stateChanged, this, [socket, this](QAbstractSocket::SocketState state){
        workerInfo(this) << "New State:" << QMetaEnum::fromType<decltype(state)>().valueToKey(state) << " <-- from:" << printClient(socket);
    });
    connect(socket, &QWebSocket::pong, this, [this, socket](quint64 elapsedTime, const QByteArray &payload){
        Q_UNUSED(elapsedTime);
        if (payload != PONG_PAYLOAD) {
            socket->close(QWebSocketProtocol::CloseCodeAbnormalDisconnection, "Corrupt Pong");
        } else {
            m_ttls[socket] = m_settings.keepalive_time;
        }
    });
    m_clients.insert(socket);
    m_ttls.insert(socket, m_settings.keepalive_time);
}

void Server::onTextMsg(const QString &data)
{
    auto err = QJsonParseError();
    auto json = JsonDict::fromJson(data.toUtf8(), &err);
    if (err.error == QJsonParseError::NoError) {
        emit sendBasic(json);
    } else {
        workerError(this) << "Error parsing data from client: " << printClient(sender());
    }
}

void Server::onRun()
{
    if (!m_websocketServer->listen(QHostAddress(m_settings.bind_to), m_settings.port)) {
        throw std::runtime_error("Could not bind to port: " + QString::number(m_settings.port.value).toStdString());
    }
    m_isConnected = true;
    m_pingTimer->start();
    Worker::onRun();
}

void Server::onMsg(const Radapter::WorkerMsg &msg)
{
    auto asBytes = msg.toBytes();
    for (auto client: qAsConst(m_clients)) {
        client->sendTextMessage(asBytes);
    }
}

