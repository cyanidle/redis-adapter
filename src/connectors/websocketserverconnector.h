#ifndef WEBSOCKETSERVERCONNECTOR_H
#define WEBSOCKETSERVERCONNECTOR_H

#include <QObject>
#include <QThread>
#include "jsondict/jsondict.hpp"
#include "websocket/websocketserver.h"
#include "broker/worker/worker.h"
#include "settings/settings.h"

namespace Websocket {
class RADAPTER_SHARED_SRC ServerConnector : public Radapter::Worker
{
    Q_OBJECT
public:
    explicit ServerConnector(const Settings::WebsocketServerInfo &serverInfo, QThread *thread);
    const Settings::WebsocketServerInfo &info() const;
signals:
    void jsonPublished(const QVariant &nestedJson);
public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
private slots:
    void onRequestReceived(const QVariant &nestedJson);
private:
    Settings::WebsocketServerInfo m_info;
    Websocket::Server* m_server;
};
}

#endif // WEBSOCKETSERVERCONNECTOR_H