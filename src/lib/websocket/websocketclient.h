#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QUrl>
#include <QJsonObject>
#include <QQueue>
#include <QTimer>
#include "JsonFormatters"
#include "radapter-broker/workerbase.h"

namespace Websocket {
    class Client;
}

class Websocket::Client : public Radapter::WorkerBase
{
    Q_OBJECT
public:
    explicit Client(const QString &serverHost,
                    quint16 serverPort,
                    const Radapter::WorkerSettings &settings, QThread *thread);
    ~Client() override;
    bool isRunning() const;

public slots:
    void start();

private slots:
    void run() override;
    void onConnected();
    void onDisconnected();
    void onError();
    void doPing();
    void onPong(quint64 elapsedTime, const QByteArray &payload);
    void onSocketReceived(const QString &message);
    void onConnectionLost();
    void startReconnect();
    void stopReconnect();
    void startHeartbeat();
    void stopHeartbeat();

private:
    void doRun();
    void doStop();

    void init();
    void initTimers();
    void resetHeartbeatCounter();

    QUrl m_serverUrl;
    QString m_serverHost;
    quint16 m_serverPort;
    QString m_userName;
    QWebSocket* m_sock;
    qint32 m_reconnectTimeout;
    QTimer* m_reconnectTimer;
    QTimer* m_heartbeatTimer;
    qint32 m_heartbeatCounter;
    QTimer* m_connectionLostTimer;
};

#endif // WEBSOCKETCLIENT_H
