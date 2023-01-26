#include "websocketserverconnector.h"
#include "redis-adapter/formatters/streamentriesmapformatter.h"
#include "radapter-broker/broker.h"

using namespace Websocket;

ServerConnector::ServerConnector(const Settings::WebsocketServerInfo &serverInfo,
                                 const Radapter::WorkerSettings &settings,
                                 QThread *thread) :
    WorkerBase(settings, thread),
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

void ServerConnector::onRun()
{
    thread()->start();
}

void ServerConnector::onMsg(const Radapter::WorkerMsg &msg)
{
    if (msg.isEmpty()) {
        return;
    }
    auto nestedJson = msg.json();
    if (StreamEntriesMapFormatter::isValid(msg)) {
        nestedJson = StreamEntriesMapFormatter(msg).joinToLatest();
    }
    nestedJson = nestedJson.nest();
    emit jsonPublished({nestedJson});
}

void ServerConnector::onRequestReceived(const QVariant &nestedJson)
{
    auto msg = prepareMsg(JsonDict(nestedJson));
    if (!msg.isEmpty()) {
        emit sendMsg(msg);
    }
}
