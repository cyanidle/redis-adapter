#ifndef REDISCONNECTOR_H
#define REDISCONNECTOR_H

#include <QObject>
#include <QTimer>
#include <QStack>
#include "lib/hiredis/adapters/qt.h"
#include <QFuture>
#include "radapter-broker/workerbase.h"

namespace Redis{

class RADAPTER_SHARED_SRC   Connector : public Radapter::WorkerBase
{
    Q_OBJECT
public:
    template <class User, class Data>
                          using MethodCbWithData = void(User::*)(redisReply* reply, Data* optData);
    template <class User> using MethodCb = void(User::*)(redisReply* reply);
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
    template <class User, class Data>
                          int runAsyncCommand(MethodCbWithData<User, Data> callback, const QString &command, Data* data);
    template <class User> int runAsyncCommand(MethodCb<User> callback, const QString &command);
                          int runAsyncCommand(StaticCb callback, const QString &command, void* optData = nullptr);

    void enablePingKeepalive();
    void disablePingKeepalive();
    void allowSelectDb();
    void blockSelectDb();

    void setConnected(bool state);
    bool isBlocked() const;
    bool isValidContext();
    bool isNullReply(redisReply *reply);
    bool isEmptyReply(redisReply *replyPtr);
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

private slots:
    void onRun() override;
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
    template <class User> inline static User* getSender(redisAsyncContext* ctx);
    const redisAsyncContext* context() const {return m_redisContext;}
    void finishAsyncCommand();
private:
    template <typename CallbackArgs_t, typename Callback>
    int runAsyncCommandImplementation(Callback callback, const QString &command, CallbackArgs_t* optData);

    template <class User, class Data>
                          static void privateCallbackWithData(redisAsyncContext* ctx, void* reply, void* data);
    template <class User> static void privateCallback(redisAsyncContext* ctx, void* reply, void* data);
                          static void privateCallbackStatic(redisAsyncContext *context, void *replyPtr, void *data);

    static void pingCallback(redisAsyncContext *context, void *replyPtr, void *sender);
    void selectCallback(redisReply *replyPtr);
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
    QMetaObject::Connection m_pingConnection;
    static timeval timeout;

                          struct CallbackArgsPlain {StaticCb callback; void *data;};
    template <class User, class Data>
                          struct CallbackArgsWithData {MethodCbWithData<User, Data> callback; Data *data;};
    template <class User> struct CallbackArgs {MethodCb<User> callback;};
    template <typename CallbackArgs_t, typename ...Args> static inline CallbackArgs_t* connAlloc(Args... args);
    template <typename CallbackArgs_t> static inline void connDealloc(CallbackArgs_t* ptr);
};

template<typename User>
inline User* Connector::getSender(redisAsyncContext* ctx) {
    static_assert(std::is_base_of<Connector, User>::value, "Cannot cast context owner to non Redis::Connector subclass!");
    return static_cast<User*>(ctx->data);
}
template<typename User, typename Data>
int Connector::runAsyncCommand(MethodCbWithData<User, Data> callback, const QString &command, Data* data) {
    return runAsyncCommandImplementation<CallbackArgsWithData<User, Data>>(
                                         privateCallbackWithData<User, Data>,
                                         command,
                                         connAlloc<CallbackArgsWithData<User, Data>>(callback, data));
}
template<typename User>
int Connector::runAsyncCommand(MethodCb<User> callback, const QString &command) {
    return runAsyncCommandImplementation<CallbackArgs<User>>(
                                         privateCallback<User>,
                                         command,
                                         connAlloc<CallbackArgs<User>>(callback));
}

template <typename CallbackArgs_t, typename Callback>
int Connector::runAsyncCommandImplementation(Callback callback, const QString &command, CallbackArgs_t* cbData) {
    if (!isConnected() || !isValidContext(m_redisContext)) {
        connDealloc(cbData);
        return REDIS_ERR;
    }
    m_pingTimer->stop();
    auto status = redisAsyncCommand(m_redisContext, callback, cbData, command.toStdString().c_str());
    if (status == REDIS_OK) {
        ++m_pendingCommandsCounter;
    } else {
        connDealloc(cbData);
    }
    return status;
}

template<typename User, typename Data>
void Connector::privateCallbackWithData(redisAsyncContext* ctx, void* reply, void* data) {
    auto args = static_cast<CallbackArgsWithData<User, Data>*>(data);
    (getSender<User>(ctx)->*(args->callback))(static_cast<redisReply*>(reply), static_cast<Data*>(args->data));
    getSender<User>(ctx)->finishAsyncCommand();
    connDealloc(args);
}
template<typename User>
void Connector::privateCallback(redisAsyncContext* ctx, void* reply, void* data) {
    auto args = static_cast<CallbackArgs<User>*>(data);
    (getSender<User>(ctx)->*(args->callback))(static_cast<redisReply*>(reply));
    getSender<User>(ctx)->finishAsyncCommand();
    connDealloc(args);
}

template <typename CallbackArgs_t, typename ...Args> inline CallbackArgs_t* Connector::connAlloc(Args... args)
{
    return new CallbackArgs_t{args...};
}

template <typename CallbackArgs_t> inline void Connector::connDealloc(CallbackArgs_t* ptr)
{
    delete ptr;
}

};

#endif // REDISCONNECTOR_H
