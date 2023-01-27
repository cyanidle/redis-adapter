#ifndef REDISCONNECTOR_H
#define REDISCONNECTOR_H

#include <QObject>
#include <QTimer>
#include <QStack>
#include "lib/hiredis/adapters/qt.h"
#include <QFuture>
#include "radapter-broker/workerbase.h"
#include "redis-adapter/commands/rediscommands.h"
#include "redis-adapter/replies/redisreplies.h"

namespace Redis {

class RADAPTER_SHARED_SRC Connector : public Radapter::WorkerBase
{
    Q_OBJECT
public:
    enum ReplyTypes {
        Unknown = 0,
        ReplyString = REDIS_REPLY_STRING,
        ReplyArray = REDIS_REPLY_ARRAY,
        ReplyInteger = REDIS_REPLY_INTEGER,
        ReplyNil = REDIS_REPLY_NIL,
        ReplyStatus = REDIS_REPLY_STATUS,
        ReplyError = REDIS_REPLY_ERROR,
        ReplyDouble = REDIS_REPLY_DOUBLE,
        ReplyBool = REDIS_REPLY_BOOL,
        ReplyMap = REDIS_REPLY_MAP,
        ReplySet = REDIS_REPLY_SET,
        ReplyAttr = REDIS_REPLY_ATTR,
        ReplyPush = REDIS_REPLY_PUSH,
        ReplyBignum = REDIS_REPLY_BIGNUM,
        ReplyVerb = REDIS_REPLY_VERB
    };
    Q_ENUM(ReplyTypes)
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
                          int runAsyncCommand(MethodCbWithData<User, Data> callback, const QString &command, Data* data, bool needBypassTracking = false);
    template <class User> int runAsyncCommand(MethodCb<User> callback, const QString &command, bool needBypassTracking = false);
                          int runAsyncCommand(StaticCb callback, const QString &command, void* optData = nullptr, bool needBypassTracking = false);


    bool isValidContext();
    static bool isValidReply(redisReply *reply);
    static bool isValidContext(const redisAsyncContext *context);
    static quint16 port(const redisAsyncContext *context);
    static QString metaInfo(const redisAsyncContext *context, const int connectionPort = -1, const QString &id = QString{});
    QString metaInfo() const;
    QString id() const;
    static QString toString(const redisReply *reply);
    void setCommandTimeout(int milliseconds);
signals:
    void connected();
    void disconnected();
    void commandsFinished();
    void alive();
public slots:
    void confirmAlive();
protected slots:
    void onRun() override;
    void resetCommandTimeout();
    void tryConnect();
    void reconnect();
    void doPing();
    void clearContext();
    void resetReconnectTimeout();
    void increaseReconnectTimeout();
    void startReconnectTimer();
    void stopReconnectTimer();
    void registerCommandTimeout();
    void resetErrorCounter();
    void startCommandTimer();
    void stopCommandTimer();
    void selectDb();
protected:
    void setConnected(bool state, const QString &reason = {});
    void enablePingKeepalive();
    void disablePingKeepalive();
    void allowSelectDb();
    void blockSelectDb();

    static QString replyTypeToString(const int replyType);
    static QString toHex(const void *pointer);
    static QString toHex(const quintptr &pointer);
    static quintptr toUintPointer(const void *pointer);

    template <class User> inline static User* getSender(redisAsyncContext* ctx);
    const redisAsyncContext* context() const {return m_redisContext;}
    void finishAsyncCommand();
    void nullifyContext();
    QVariant parseReply(redisReply *reply);
private:
    template <class User, class Data>
    static void privateCallbackWithData(redisAsyncContext* ctx, void* reply, void* data);
    template <class User>
    static void privateCallback(redisAsyncContext* ctx, void* reply, void* data);
    static void privateCallbackStatic(redisAsyncContext *context, void *replyPtr, void *data);
    static void pingCallback(redisAsyncContext *context, void *replyPtr, void *sender);
    static void connectCallback(const redisAsyncContext *context, int status);
    static void disconnectCallback(const redisAsyncContext *context, int status);

    template <typename CallbackArgs_t, typename Callback>
    int runAsyncCommandImplementation(Callback callback, const QString &command, CallbackArgs_t* optData, bool needTrackingBypass = false);
    void selectCallback(redisReply *replyPtr);

    QTimer* m_pingTimer;
    quint32 m_pendingCommandsCounter{0u};
    redisAsyncContext* m_redisContext;
    QTimer* m_reconnectTimer;
    QTimer* m_commandTimer;
    bool m_isConnected;
    quint8 m_commandTimeoutsCounter;
    RedisQtAdapter* m_client;
    QString m_host;
    quint16 m_port;
    quint16 m_dbIndex;
    bool m_canSelect;
    struct CallbackArgsPlain {
        StaticCb callback;
        void *data;
    };
    template <class User, class Data>                      
    struct CallbackArgsWithData {
        MethodCbWithData<User, Data> callback;
        Data *data;
    };
    template <class User> struct CallbackArgs {
        MethodCb<User> callback;
    };
    template <typename CallbackArgs_t, typename ...Args>
    static CallbackArgs_t* connAlloc(Args... args);
    template <typename CallbackArgs_t>
    static void connDealloc(CallbackArgs_t* ptr);
};


template<typename User>
inline User* Connector::getSender(redisAsyncContext* ctx) {
    static_assert(std::is_base_of<Connector, User>::value, "Cannot cast context owner to non Redis::Connector subclass!");
    return qobject_cast<User*>(static_cast<Connector*>(ctx->data));
}
template<typename User, typename Data>
int Connector::runAsyncCommand(MethodCbWithData<User, Data> callback, const QString &command, Data* data, bool needBypassTracking) {
    return runAsyncCommandImplementation<CallbackArgsWithData<User, Data>>(
                                         privateCallbackWithData<User, Data>,
                                         command,
                                         connAlloc<CallbackArgsWithData<User, Data>>(callback, data),
                                         needBypassTracking);
}
template<typename User>
int Connector::runAsyncCommand(MethodCb<User> callback, const QString &command, bool needBypassTracking) {
    return runAsyncCommandImplementation<CallbackArgs<User>>(
                                         privateCallback<User>,
                                         command,
                                         connAlloc<CallbackArgs<User>>(callback),
                                         needBypassTracking);
}

template <typename CallbackArgs_t, typename Callback>
int Connector::runAsyncCommandImplementation(Callback callback, const QString &command, CallbackArgs_t* cbData, bool needTrackingBypass) {
    if (!isConnected() || !isValidContext(m_redisContext)) {
        connDealloc(cbData);
        return REDIS_ERR;
    }
    m_pingTimer->stop();
    auto status = redisAsyncCommand(m_redisContext, callback, cbData, command.toStdString().c_str());
    if (status == REDIS_OK) {
        if (!needTrackingBypass) {
            startCommandTimer();
            ++m_pendingCommandsCounter;
        }
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

}

Q_DECLARE_METATYPE(Redis::Connector::ReplyTypes)

#endif // REDISCONNECTOR_H
