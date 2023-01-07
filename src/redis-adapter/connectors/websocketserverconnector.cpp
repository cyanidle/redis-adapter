#include "websocketserverconnector.h"
#include "redis-adapter/formatters/streamentriesmapformatter.h"
#include "radapter-broker/broker.h"

using namespace Websocket;

ServerConnector::ServerConnector(const Settings::WebsocketServerInfo &serverInfo,
                                 const Radapter::WorkerSettings &settings,
                                 QThread *thread)
    : WorkerBase(settings, thread),
      m_info(serverInfo)
{
    m_thread = new QThread();
    m_server = serverInfo.port > 0u ? new Websocket::Server(serverInfo.port)
                                    : new Websocket::Server();
    m_server->moveToThread(m_thread);
    connect(m_thread, &QThread::started, m_server, &Websocket::Server::start);
    connect(m_thread, &QThread::finished, m_server, &Websocket::Server::deleteLater);
    connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);

    connect(this, &ServerConnector::jsonPublished,
            m_server, &Websocket::Server::publishNewData,
            Qt::QueuedConnection);
    connect(m_server, &Websocket::Server::requestReceived,
            this, &ServerConnector::onRequestReceived,
            Qt::QueuedConnection);
}

ServerConnector &ServerConnector::prvInstance(const Settings::WebsocketServerInfo &serverInfo,
                                              const Radapter::WorkerSettings &settings,
                                              QThread *thread)
{
    static ServerConnector connector(serverInfo, settings, thread);
    return connector;
}

ServerConnector *ServerConnector::instance()
{
    return &prvInstance();
}

void ServerConnector::init(const Settings::WebsocketServerInfo &serverInfo,
                           const Radapter::WorkerSettings &settings,
                           QThread *thread)
{
    prvInstance(serverInfo, settings, thread);
}

Settings::WebsocketServerInfo ServerConnector::info() const
{
    return m_info;
}

void ServerConnector::run()
{
    m_thread->start();
    thread()->start();
}

void ServerConnector::onMsg(const Radapter::WorkerMsg &msg)
{
    if (msg.isEmpty()) {
        return;
    }
    auto nestedJson = msg.json();
    if (StreamEntriesMapFormatter::isValid(msg.data())) {
        nestedJson = StreamEntriesMapFormatter(msg.data()).joinToLatest();
    }
    nestedJson = nestedJson.nest();
    emit jsonPublished(nestedJson.data());
}

void ServerConnector::onRequestReceived(const QVariant &nestedJson)
{
    auto msg = prepareMsg( JsonDict(nestedJson));
    if (!msg.isEmpty()) {
        emit sendMsg(msg);
    }
}
