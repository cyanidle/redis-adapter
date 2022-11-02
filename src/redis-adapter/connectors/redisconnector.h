#ifndef REDISCONNECTOR_H
#define REDISCONNECTOR_H

#include <QObject>
#include <QTimer>
#include <QStack>
#include "lib/hiredis/adapters/qt.h"
#include "radapter-broker/workerbase.h"

namespace Redis{
class RADAPTER_SHARED_SRC Connector;
}

class Redis::Connector : public Radapter::WorkerBase
{
    Q_OBJECT
public:
    struct CallbackArgs {
        Connector* sender;
        QVariant args;
    };

    explicit Connector(const QString &host,
                            const quint16 port,
                            const quint16 dbIndex,
                            const Radapter::WorkerSettings &settings);
    ~Connector() override;
    QString host() const;
    quint16 port() const;
    quint16 dbIndex() const;
    void setDbIndex(const quint16 dbIndex);
    bool isConnected() const;

    int runAsyncCommand(const QString &command);
    //! \warning If args is a valid Variant, then reintepret third args in callback as CallbackArgs*, then delete!
    int runAsyncCommand(redisCallbackFn *callback, const QString &command, const QVariant &args = QVariant());

    void enablePingKeepalive();
    void disablePingKeepalive();
    void allowSelectDb();
    void blockSelectDb();

    void setConnected(bool state);
    bool isBlocked() const;
    static bool isValidContext(const redisAsyncContext *context);
    static bool isNullReply(redisAsyncContext *context, void *replyPtr, void *sender);
    static bool isEmptyReply(redisAsyncContext *context, void *replyPtr);
    static quint16 port(const redisAsyncContext *context);
    static std::string metaInfo(const redisAsyncContext *context, const int connectionPort = -1, const QString &id = QString{});
    std::string metaInfo() const;
    virtual QString id() const;
    static QString toString(const redisReply *reply);
signals:
    void connected();
    void disconnected();
    void commandsFinished();

public slots:
    virtual void run() override;
    void finishAsyncCommand();

private slots:
    void tryConnect();
    void doPing();
    void clearContext();
    void nullifyContext();
    void resetConnectionTimeout();
    void increaseConnectionTimeout();
    void stopConnectionTimer();
    void incrementErrorCounter();
    void resetErrorCounter();
    void stopCommandTimer();
    void selectDb();

private:
    static void pingCallback(redisAsyncContext *context, void *replyPtr, void *sender);
    static void selectCallback(redisAsyncContext *context, void *replyPtr, void *sender);
    static void connectCallback(const redisAsyncContext *context, int status);
    static void disconnectCallback(const redisAsyncContext *context, int status);

    QTimer* m_connectionTimer;
    QTimer* m_pingTimer;
    QTimer* m_commandTimer;
    QTimer* m_reconnectCooldown;
    bool m_isConnected;
    quint8 m_commandTimeoutsCounter;
    QStack<redisCallbackFn *> m_commandStack;
    redisAsyncContext* m_redisContext;
    RedisQtAdapter* m_client;
    QString m_host;
    quint16 m_port;
    quint16 m_dbIndex;
    bool m_canSelect;
};

#endif // REDISCONNECTOR_H
