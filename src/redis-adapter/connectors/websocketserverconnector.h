#ifndef WEBSOCKETSERVERCONNECTOR_H
#define WEBSOCKETSERVERCONNECTOR_H

#include <QObject>
#include <QThread>
#include "jsondict/jsondict.hpp"
#include "lib/websocket/websocketserver.h"
#include "radapter-broker/workerbase.h"
#include "redis-adapter/settings/settings.h"

namespace Websocket {
class RADAPTER_SHARED_SRC ServerConnector : public Radapter::WorkerBase
{
    Q_OBJECT
public:
    explicit ServerConnector(const Settings::WebsocketServerInfo &serverInfo,
                             const Radapter::WorkerSettings &settings,
                             QThread *thread);
    const Settings::WebsocketServerInfo &info() const;
signals:
    void jsonPublished(const QVariant &nestedJson);
public slots:
    void onRun() override;
    void onMsg(const Radapter::WorkerMsg &msg) override;
private slots:
    void onRequestReceived(const QVariant &nestedJson);
private:
    Settings::WebsocketServerInfo m_info;
    Websocket::Server* m_server;
};
}

#endif // WEBSOCKETSERVERCONNECTOR_H
