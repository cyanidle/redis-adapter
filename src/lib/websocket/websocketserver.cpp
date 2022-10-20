#include "websocketserver.h"
#include <QTimer>
#include <QJsonDocument>
#include "websocketconstants.h"
#include "redis-adapter/radapterlogging.h"

using namespace Websocket;

#define RECONNECT_TIME_MS   10000
#define FILTRATION_DELAY_MS 5000
#define INIT_SEND_DELAY_MS  60000 // 60 seconds

#define KEY_CLIENTS_ONLINE  "clients_online"

Server::Server(quint16 port, QObject *parent)
    : QObject(parent),
      m_websocketServer(nullptr),
      m_clients{},
      m_localData{},
      m_persistentDataInited(false)
{
    m_port = port > 0u ? port : DEFAULT_WS_PORT;
}

Server::~Server()
{
    close();
    delete m_fullDataSender;
}

void Server::start()
{
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->setInterval(RECONNECT_TIME_MS);
    m_reconnectTimer->callOnTimeout(this, &Server::doRun);

    m_fullDataSender = new QTimer(this);
    m_fullDataSender->setSingleShot(false);
    m_fullDataSender->setInterval(INIT_SEND_DELAY_MS);
    m_fullDataSender->callOnTimeout(this, &Server::publishFullData);

    connect(this, &Server::clientsAmountChanged,
            this, &Server::publishClientsCounter);
    connect(this, &Server::deviceDataReceived,
            this, &Server::publishNewData);

    initData();
    doRun();
}

void Server::doRun()
{
    if (m_websocketServer == nullptr) {
        m_websocketServer = new QWebSocketServer("LotosGate", QWebSocketServer::NonSecureMode, this);
    }

    if (m_websocketServer->listen(QHostAddress::Any, m_port)) {
        wseDebug() << "Lotos websocket server listening on port" << m_port;
        connect(m_websocketServer, &QWebSocketServer::newConnection,
                this, &Server::connectClient);
        connect(m_websocketServer, &QWebSocketServer::closed,
                this, &Server::reconnect);
    } else {
        wseDebug() << "Error! Lotos websocket server cannot be inited on port" << m_port;
        wseDebug() << m_websocketServer->error() << m_websocketServer->errorString();
    }
}

void Server::close()
{
    m_websocketServer->close();
    delete m_websocketServer;
    qDeleteAll(m_clients);
    m_fullDataSender->stop();
}

void Server::initData()
{
    emit dataInitRequested();
}

quint8 Server::clientsCounter() const
{
    auto counter = static_cast<quint8>(m_clients.count());
    return counter;
}

void Server::reconnect()
{
    close();
    m_reconnectTimer->start();
}

void Server::connectClient()
{
    auto socket = m_websocketServer->nextPendingConnection();
    if (socket) {
        connect(socket, &QWebSocket::disconnected,
                this, &Server::disconnectClient);
        connect(socket, &QWebSocket::textMessageReceived,
                this, &Server::receiveClientData);
        addClient(socket);
        initClient(socket);

        // load persistent data on first client
        if (!m_persistentDataInited) {
            emit persistentDataRequested();
            wseDebug() << Q_FUNC_INFO << "persistent data requested";
            m_persistentDataInited = true;
        }

        if (!m_fullDataSender->isActive()) {
            m_fullDataSender->start();
        }
    }
}

void Server::disconnectClient()
{
    auto client = qobject_cast<QWebSocket*>(sender());
    if (client) {
        removeClient(client);
        client->deleteLater();
    }
}

void Server::initClient(QWebSocket *client)
{
    bool ok = false;
    auto jsonFullDoc = getFullData(ok);
    if (ok) {
        sendTextMessage(client, toJson(jsonFullDoc));
        wseJsonDebug() << Q_FUNC_INFO << jsonFullDoc.toJson(QJsonDocument::JsonFormat::Indented);
    }
}

void Server::addClient(QWebSocket *client)
{
    m_clients.append(client);
    emit clientsAmountChanged(clientsCounter());
}

void Server::removeClient(QWebSocket *client)
{
    m_clients.removeAll(client);
    emit clientsAmountChanged(clientsCounter());
}

void Server::publishClientsCounter()
{
    auto clientsCount = QVariantMap{ { KEY_CLIENTS_ONLINE, clientsCounter() } };
    publishJson(QJsonDocument::fromVariant(clientsCount));
}

void Server::publishNewData(const QVariant &formattedData)
{
    auto jsonDoc = QJsonDocument::fromVariant(formattedData);
    auto newData = jsonDoc.object();
    if (newData.isEmpty()) {
        return;
    }

    updateLocalData(newData);
    publishJson(jsonDoc);
}

void Server::publishFullData()
{
    bool ok = false;
    auto jsonFullData = getFullData(ok);
    if (ok) {
        publishJson(jsonFullData);
    }
}

void Server::receiveClientData(const QString &message)
{
    if (message.isEmpty()) {
        return;
    }

    auto jsonDoc = QJsonDocument::fromJson(message.toUtf8());
    if (jsonDoc.isNull() || jsonDoc.isEmpty()) {
        return;
    }
    auto formattedData = jsonDoc.toVariant();
    emit requestReceived(formattedData);
}

void Server::publishJson(const QJsonDocument &document)
{
    auto message = toJson(document);
    publishText(message);
    wseJsonDebug() << Q_FUNC_INFO << "new data:" << document.toJson(QJsonDocument::Indented);
}

void Server::publishText(const QString &text)
{
    for (auto client : qAsConst(m_clients)) {
        sendTextMessage(client, text);
    }
}

void Server::sendTextMessage(QWebSocket *client, const QString &text)
{
    auto bytesSent = client->sendTextMessage(text);
    wseDebug() << "Websocket client" << client << "sent:" << bytesSent;
    auto hasFlushed = client->flush();
    wseDebug() << "Websocket client" << client << "flushed:" << hasFlushed;
}

QString Server::toJson(const QJsonDocument &document) const
{
    auto text = document.toJson(QJsonDocument::Compact);
    return text;
}

void Server::updateLocalData(const QJsonObject &newData)
{
    mergeJsonObjects(m_localData, newData);
}

void Server::mergeJsonObjects(QJsonObject &destObject, const QJsonObject &srcObject)
{
    for (auto srcIterator = srcObject.begin();
         srcIterator != srcObject.end();
         srcIterator++)
    {
        if (srcIterator.value().isObject()) {
            auto destSubObject = destObject[srcIterator.key()].toObject();
            auto srcSubObject = srcIterator.value().toObject();
            mergeJsonObjects(destSubObject, srcSubObject);
            if (!destSubObject.isEmpty()) {
                destObject.insert(srcIterator.key(), destSubObject);
            }
        } else {
            destObject.insert(srcIterator.key(), srcIterator.value());
        }
    }
}

QJsonDocument Server::getFullData(bool &ok) const
{
    auto jsonDoc = QJsonDocument(m_localData);
    ok = !jsonDoc.isNull() && !jsonDoc.object().isEmpty();
    return jsonDoc;
}
