#include "websocketserverconnector.h"

using namespace Websocket;

ServerConnector::ServerConnector(const Settings::WebsocketServerInfo &serverInfo, QThread *thread) :
    Worker(serverInfo.worker, thread),
    m_info(serverInfo)
{
    new Websocket::Server(serverInfo.port, this);
    connect(thread, &QThread::started, m_server, &Websocket::Server::start);
    connect(thread, &QThread::finished, m_server, &Websocket::Server::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    connect(this, &ServerConnector::jsonPublished,
            m_server, &Websocket::Server::publishNewData,
            Qt::QueuedConnection);
    connect(m_server, &Websocket::Server::requestReceived,
            this, &ServerConnector::onRequestReceived,
            Qt::QueuedConnection);
}

const Settings::WebsocketServerInfo &ServerConnector::info() const
{
    return m_info;
}

void ServerConnector::onMsg(const Radapter::WorkerMsg &msg)
{
    if (msg.isEmpty()) {
        return;
    }
    emit jsonPublished(msg.toVariant());
}

void ServerConnector::onRequestReceived(const QVariant &nestedJson)
{
    auto msg = prepareMsg(JsonDict(nestedJson));
    if (!msg.isEmpty()) {
        emit sendMsg(msg);
    }
}
