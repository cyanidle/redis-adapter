#ifndef WEBSOCKETCLIENTFACTORY_H
#define WEBSOCKETCLIENTFACTORY_H

#include <QObject>
#include <QThread>
#include <QList>
#include "lib/websocket/websocketclient.h"
#include "radapter-broker/factorybase.h"
#include "redis-adapter/settings/settings.h"

namespace Websocket {
class RADAPTER_SHARED_SRC ClientFactory;
}

class Websocket::ClientFactory : public Radapter::FactoryBase
{
    Q_OBJECT
public:
    explicit ClientFactory(const QList<Settings::WebsocketClientInfo> &info,
                                    QObject *parent = nullptr);
    int initWorkers() override;
    int initSettings() override {return 0;}
    void run() override;
    Radapter::WorkersList getWorkers() const;
signals:

private:
    QList<Settings::WebsocketClientInfo> m_info;
    Radapter::WorkersList m_workersPool;
};

#endif // WEBSOCKETCLIENTFACTORY_H
