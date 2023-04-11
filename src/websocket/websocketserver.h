#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include "broker/workers/worker.h"
#include "settings/settings.h"

class QTimer;
class QWebSocketServer;
class QWebSocket;
namespace Websocket {

class RADAPTER_API Server : public Radapter::Worker
{
    Q_OBJECT
public:
    explicit Server(const Settings::WebsocketServer &settings, QThread *thread);
    bool isConnected() const;
    ~Server() override;
private slots:
    void onRun() override;
    void onMsg(const Radapter::WorkerMsg& msg) override;
    void onNewConnection();
    void onTextMsg(const QString &data);
    void checkClients();
private:
    void reconnect();

    QTimer *m_pingTimer;
    QWebSocketServer* m_websocketServer;
    QSet<QWebSocket*> m_clients;
    QHash<QWebSocket*, int> m_ttls;
    Settings::WebsocketServer m_settings;
    std::atomic<bool> m_isConnected;
};

}

#endif // LOTOSGATESERVER_H
