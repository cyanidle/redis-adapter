#ifndef WEBSOCKETSERVERCONNECTOR_H
#define WEBSOCKETSERVERCONNECTOR_H

#include <QObject>
#include <QThread>
#include "json-formatters/formatters/dict.h"
#include "lib/websocket/websocketserver.h"
#include "radapter-broker/singletonbase.h"
#include "redis-adapter/settings/settings.h"

namespace Websocket {
class RADAPTER_SHARED_SRC ServerConnector;
}

class Websocket::ServerConnector : public Radapter::SingletonBase
{
    Q_OBJECT
public:
    static ServerConnector* instance();
    static void init(const Settings::WebsocketServerInfo &serverInfo,
                     const Radapter::WorkerSettings &settings);

    Settings::WebsocketServerInfo info() const;
    Radapter::WorkerMsg::SenderType workerType() const override {return Radapter::WorkerMsg::TypeWebsockerServerConnector;}
signals:
    void jsonPublished(const QVariant &nestedJson);
public slots:
    int initSettings() override {return 0;}
    int init() override {return 0;}
    void run() override;
    void onMsg(const Radapter::WorkerMsg &msg) override;

private slots:
    void onRequestReceived(const QVariant &nestedJson);

private:
    //Replies

    explicit ServerConnector(const Settings::WebsocketServerInfo &serverInfo,
                                      const Radapter::WorkerSettings &settings);
    static ServerConnector& prvInstance(const Settings::WebsocketServerInfo &serverInfo = {},
                                                 const Radapter::WorkerSettings &settings = {});
    Settings::WebsocketServerInfo m_info;
    Websocket::Server* m_server;
    QThread* m_thread;
};

#endif // WEBSOCKETSERVERCONNECTOR_H
