#ifndef REDISCONNECTOR_H
#define REDISCONNECTOR_H

#include <QObject>
#include <QTimer>
#include <QStack>
#include "lib/hiredis/adapters/qt.h"
#include "radapter-broker/workerbase.h"

namespace Redis{

class RADAPTER_SHARED_SRC Connector : public Radapter::WorkerBase
{
    Q_OBJECT
public:
    template <class User> using MethodCbWithData = void(User::*)(redisAsyncContext* ctx, redisReply* reply, void* optData);
    template <class User> using MethodCb = void(User::*)(redisAsyncContext* ctx, redisReply* reply);
                          using StaticCb = void(*)(redisAsyncContext* ctx, void* reply, void* sender);


    explicit Connector(const QString &host,
                       const quint16 port,
                       const quint16 dbIndex,
                       const Radapter::WorkerSettings &settings,
                       QThread *thread);
    ~Connector() override;
    QString host() const;
    quint16 port() const;
    quint16 dbIndex() const;
    void setDbIndex(const quint16 dbIndex);
    bool isConnected() const;

    int runAsyncCommand(const QString &command);

    void enablePingKeepalive();
    void disablePingKeepalive();
    void allowSelectDb();
    void blockSelectDb();

    void setConnected(bool state);
    bool isBlocked() const;
    static bool isValidContext(const redisAsyncContext *context);
    static bool isNullReply(redisAsyncContext *context, void *replyPtr);
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
protected:
    template <class User> int runAsyncCommand(MethodCbWithData<User> callback, const QString &command, void* data);
    template <class User> int runAsyncCommand(MethodCb<User> callback, const QString &command);
    int runAsyncCommand(StaticCb callback, const QString &command, void* optData = nullptr);
    void finishAsyncCommand();

    void *enqueueMsg(const Radapter::WorkerMsg &msg);
    Radapter::WorkerMsg dequeueMsg(void* msgId);
    void disposeId(void *msgId);
private:
    template <class User> inline static User* getThis(redisAsyncContext* ctx);
    template <class User> static void privateCallbackWithData(redisAsyncContext* ctx, void* reply, void* optData);
    template <class User> static void privateCallback(redisAsyncContext* ctx, void* reply, void* optData);
    static void privateCallbackPlain(redisAsyncContext *context, void *replyPtr, void *data);

    static void pingCallback(redisAsyncContext *context, void *replyPtr, void *sender);
    void selectCallback(redisAsyncContext *context, redisReply *replyPtr);
    static void connectCallback(const redisAsyncContext *context, int status);
    static void disconnectCallback(const redisAsyncContext *context, int status);

    QTimer* m_pingTimer;
    quint32 m_pendingCommandsCounter;
    redisAsyncContext* m_redisContext;
    QTimer* m_connectionTimer;
    QTimer* m_commandTimer;
    QTimer* m_reconnectCooldown;
    bool m_isConnected;
    quint8 m_commandTimeoutsCounter;
    RedisQtAdapter* m_client;
    QString m_host;
    quint16 m_port;
    quint16 m_dbIndex;
    bool m_canSelect;
    QHash<quint64, Radapter::WorkerMsg> m_queue;

    struct CallbackArgsPlain {
        StaticCb callback;
        void *data;
    };
    template <class User>
    struct CallbackArgsWithData {
        MethodCbWithData<User> callback;
        void *data;
    };
    template <class User>
    struct CallbackArgs {
        MethodCb<User> callback;
    };
};



template<typename User>
inline User* Connector::getThis(redisAsyncContext* ctx) {
    return static_cast<User*>(ctx->data);
}
template<typename User>
int Connector::runAsyncCommand(MethodCbWithData<User> callback, const QString &command, void* data) {
    if (!isConnected() || !isValidContext(m_redisContext)) {
        return REDIS_ERR;
    }
    m_pingTimer->stop();
    auto status = redisAsyncCommand(m_redisContext, privateCallbackWithData<User>,
                                    new CallbackArgsWithData<User>{callback, data},
                                    command.toStdString().c_str());
    if (status == REDIS_OK) {
        ++m_pendingCommandsCounter;
    }
    return status;
}
template<typename User>
int Connector::runAsyncCommand(MethodCb<User> callback, const QString &command) {
    if (!isConnected() || !isValidContext(m_redisContext)) {
        return REDIS_ERR;
    }
    m_pingTimer->stop();
    auto status = redisAsyncCommand(m_redisContext, privateCallback<User>,
                                    new CallbackArgs<User>{callback},
                                    command.toStdString().c_str());
    if (status == REDIS_OK) {
        ++m_pendingCommandsCounter;
    }
    return status;
}
template<typename User>
void Connector::privateCallbackWithData(redisAsyncContext* ctx, void* reply, void* optData) {
    auto args = static_cast<CallbackArgsWithData<User>*>(optData);
    (getThis<User>(ctx)->*(args->callback))(ctx, static_cast<redisReply*>(reply), args->data);
    getThis<User>(ctx)->finishAsyncCommand();
    delete args;
}
template<typename User>
void Connector::privateCallback(redisAsyncContext* ctx, void* reply, void* optData) {
    auto args = static_cast<CallbackArgs<User>*>(optData);
    (getThis<User>(ctx)->*(args->callback))(ctx, static_cast<redisReply*>(reply));
    getThis<User>(ctx)->finishAsyncCommand();
    delete args;
}

};

#endif // REDISCONNECTOR_H
