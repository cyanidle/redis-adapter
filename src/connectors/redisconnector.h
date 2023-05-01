#ifndef REDISCONNECTOR_H
#define REDISCONNECTOR_H

#include "broker/workers/worker.h"
#include "lib/hiredis/adapters/qt.h"

namespace Settings {
struct RedisConnector;
}
namespace Redis {
class RADAPTER_API Connector : public Radapter::Worker
{
    Q_OBJECT
    struct Private;
public:
    template <class User, class Data>
                          using MethodCbWithData = void(User::*)(redisReply* reply, Data* optData);
    template <class User> using MethodCb = void(User::*)(redisReply* reply);
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
    explicit Connector(const Settings::RedisConnector &settings, QThread *thread);
    ~Connector() override;
    bool isConnected() const;
    void waitConnected(Radapter::Worker *who = nullptr) const;
    int commandsLeft() const;
signals:
    void connected();
    void disconnected();
    void commandsFinished();
protected slots:
    void onRun() override;
    void tryConnect();
    void reconnect();
    void doPing();
    void clearContext();
    void onCommandTimeout();
    void selectDb();
protected:
    template <class User, class Data>
    int runAsyncCommand(MethodCbWithData<User, Data> callback, const QString &command, Data* data, bool needBypassTracking = false);
    template <class User>
    int runAsyncCommand(MethodCb<User> callback, const QString &command, bool needBypassTracking = false);
    int runAsyncCommand(const QString &command);
    void setDbIndex(const quint16 dbIndex);
    QVariant parseReply(redisReply *reply) const;
    QVariantMap parseHashReply(redisReply *reply) const;
    bool isValidContext();
    void setConnected(bool state, const QString &reason = {});
    void enablePingKeepalive();
    void disablePingKeepalive();

    redisAsyncContext *context();
    const redisAsyncContext* context() const;
private:
    template <class User, class Data>
    static void privateCallbackWithData(redisAsyncContext* ctx, void* reply, void* data);
    template <class User>
    static void privateCallback(redisAsyncContext* ctx, void* reply, void* data);
    static void connectCallback(const redisAsyncContext *context, int status);
    static void disconnectCallback(const redisAsyncContext *context, int status);
    void selectCallback(redisReply *replyPtr);
    void pingCallback(redisReply *replyPtr);
    void startAsyncCommand(bool bypassTrack);
    void finishAsyncCommand();
    template <typename CallbackArgs_t, typename Callback>
    int runAsyncCommandImplementation(Callback callback, const QString &command, CallbackArgs_t* optData, bool needTrackingBypass);

    Private *d;
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
inline User* getSender(redisAsyncContext* ctx) {
    static_assert(std::is_base_of<Connector, User>::value, "Cannot cast context owner to non Redis::Connector subclass!");
    return static_cast<User*>(ctx->data);
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
    if (!isConnected() || !isValidContext()) {
        connDealloc(cbData);
        return REDIS_ERR;
    }
    auto status = redisAsyncCommand(context(), callback, cbData, command.toStdString().c_str());
    if (status != REDIS_OK) {
        connDealloc(cbData);
    } else {
        startAsyncCommand(needTrackingBypass);
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

#endif // REDISCONNECTOR_H
