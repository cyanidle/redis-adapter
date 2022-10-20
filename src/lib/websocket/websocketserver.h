#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonObject>
#include <QTimer>

namespace Websocket {
    class RADAPTER_SHARED_SRC Server;
}

class Websocket::Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(quint16 port = 0u, QObject *parent = nullptr);
    ~Server() override; 

    void start();
    void close();

    quint8 clientsCounter() const;

signals:
    void deviceDataReceived(const QVariant &formattedData);
    void requestReceived(const QVariant &formattedData);
    void clientsAmountChanged(const quint8 counter);
    void dataInitRequested();
    void persistentDataRequested();

public slots:
    void reconnect();
    void connectClient();
    void disconnectClient();
    void publishClientsCounter();
    void publishNewData(const QVariant &formattedData);
    void receiveClientData(const QString &message);
    void publishFullData();

private slots:

private:
    void initData();
    void doRun();

    void initClient(QWebSocket *client);
    void addClient(QWebSocket *client);
    void removeClient(QWebSocket *client);

    void publishJson(const QJsonDocument &document);
    void publishText(const QString &text);
    void sendTextMessage(QWebSocket *client, const QString &text);
    QString toJson(const QJsonDocument &document) const;

    void updateLocalData(const QJsonObject &newData);
    void mergeJsonObjects(QJsonObject &destObject, const QJsonObject &srcObject);

    QJsonDocument getFullData(bool &ok) const;

    quint16 m_port;
    QTimer* m_reconnectTimer;
    QTimer* m_fullDataSender;
    QWebSocketServer* m_websocketServer;
    QList<QWebSocket*> m_clients;
    QJsonObject m_localData;
    bool m_persistentDataInited;
};

#endif // LOTOSGATESERVER_H
