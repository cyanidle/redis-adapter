#include "redisconnector.h"
#include "redis-adapter/radapterlogging.h"
#ifdef _MSC_VER
#include <WinSock2.h>
#endif

#define TCP_CONNECT_TIMEOUT_MS  1000
#define MAX_TIMEOUT_FACTOR      10
#define PING_TIMEOUT_MS         10000
#define PING_REQUEST            "PING"
#define PING_REPLY              "PONG"
#define SELECT_DB_REQUEST       "SELECT"
#define COMMAND_TIMEOUT_MS      150
#define MAX_COMMAND_ERRORS      3
#define RECONNECT_DELAY_MS      1500

using namespace Redis;

Connector::Connector(const QString &host,
                     const quint16 port,
                     const quint16 dbIndex,
                     const Radapter::WorkerSettings &settings,
                     QThread *thread) :
    WorkerBase(settings, thread),
    m_pingTimer(nullptr),
    m_redisContext(nullptr),
    m_connectionTimer(nullptr),
    m_commandTimer(nullptr),
    m_reconnectCooldown(nullptr),
    m_isConnected(false),
    m_commandTimeoutsCounter{},
    m_client(nullptr),
    m_host(host),
    m_port(port),
    m_dbIndex(dbIndex),
    m_canSelect(true),
    m_pingConnection()
{
}

Connector::~Connector()
{
    if (m_isConnected) {
        redisAsyncDisconnect(m_redisContext);
    } else if (m_redisContext) {
        redisAsyncFree(m_redisContext);
    }
}

void Connector::run()
{
    m_client = new RedisQtAdapter(this);
    m_connectionTimer = new QTimer(this);
    m_connectionTimer->setSingleShot(true);
    m_connectionTimer->callOnTimeout(this, [this](){
        reError() << metaInfo().c_str() << "Connection timeout. Trying new connection...";
        increaseConnectionTimeout();
        clearContext();
        tryConnect();
    });
    resetConnectionTimeout();
    connect(this, &Connector::connected, &Connector::resetConnectionTimeout);
    connect(this, &Connector::connected, &Connector::stopConnectionTimer);

    m_reconnectCooldown = new QTimer(this);
    m_reconnectCooldown->setSingleShot(true);
    m_reconnectCooldown->setInterval(RECONNECT_DELAY_MS);
    m_reconnectCooldown->callOnTimeout(this, &Connector::tryConnect);
    connect(this, &Connector::disconnected, m_reconnectCooldown, QOverload<>::of(&QTimer::start));

    m_pingTimer = new QTimer(this);
    m_pingTimer->setSingleShot(false);
    m_pingTimer->setInterval(PING_TIMEOUT_MS);
    enablePingKeepalive();

    m_commandTimer = new QTimer(this);
    m_commandTimer->setSingleShot(true);
    m_commandTimer->setInterval(COMMAND_TIMEOUT_MS);
    m_commandTimer->callOnTimeout([this](){
        if (m_commandTimeoutsCounter < MAX_COMMAND_ERRORS) {
            reDebug() << metaInfo().c_str() << "command timeout";
        }
        incrementErrorCounter();
    });

    connect(this, &Connector::connected, &Connector::selectDb);
    tryConnect();
}

void Connector::finishAsyncCommand()
{
    if (!--m_pendingCommandsCounter) {
        m_pingTimer->start();
        emit commandsFinished();
    }
}

void Connector::tryConnect()
{
    if (isConnected()) {
        return;
    }

    auto options = redisOptions{};
    REDIS_OPTIONS_SET_TCP(&options, m_host.toStdString().c_str(), m_port);
    auto timeout = timeval{};
    timeout.tv_usec = TCP_CONNECT_TIMEOUT_MS;
    options.connect_timeout = &timeout;
    m_redisContext = redisAsyncConnectWithOptions(&options);
    m_redisContext->data = this;
    m_client->setContext(m_redisContext);
    redisAsyncSetConnectCallback(m_redisContext, connectCallback);
    redisAsyncSetDisconnectCallback(m_redisContext, disconnectCallback);
    m_connectionTimer->start();
    if (m_redisContext->err) {
        reDebug() << metaInfo().c_str() << "Error: " << m_redisContext->errstr;
        clearContext();
        return;
    }
    doPing();
}

void Connector::doPing()
{
    redisAsyncCommand(m_redisContext, pingCallback, this, PING_REQUEST);
    m_commandTimer->start();
}

void Connector::clearContext()
{
    if (m_redisContext) {
        redisAsyncDisconnect(m_redisContext);
        redisAsyncFree(m_redisContext);
        nullifyContext();
    }
}

void Connector::nullifyContext()
{
    m_redisContext = nullptr;
}

void Connector::resetConnectionTimeout()
{
    m_connectionTimer->setInterval(TCP_CONNECT_TIMEOUT_MS);
}

void Connector::increaseConnectionTimeout()
{
    auto timeout = m_connectionTimer->interval() * 2;
    if (timeout > (TCP_CONNECT_TIMEOUT_MS * MAX_TIMEOUT_FACTOR)) {
        timeout = (TCP_CONNECT_TIMEOUT_MS * MAX_TIMEOUT_FACTOR);
    }
    m_connectionTimer->setInterval(timeout);
}

void Connector::stopConnectionTimer()
{
    m_connectionTimer->stop();
}

void Connector::incrementErrorCounter()
{
    m_commandTimeoutsCounter++;
    if (m_commandTimeoutsCounter >= MAX_COMMAND_ERRORS) {
        m_commandTimeoutsCounter = MAX_COMMAND_ERRORS;
        setConnected(false);
    }
}

void Connector::resetErrorCounter()
{
    m_commandTimeoutsCounter = 0u;
}

void Connector::stopCommandTimer()
{
    m_commandTimer->stop();
}

void Connector::selectDb()
{
    if (!m_canSelect) {
        return;
    }
    auto selectCommand = QString("%1 %2").arg(SELECT_DB_REQUEST).arg(m_dbIndex);
    runAsyncCommand(&Connector::selectCallback, selectCommand);
}

void Connector::pingCallback(redisAsyncContext *context, void *replyPtr, void *sender)
{
    auto adapter = static_cast<Connector *>(sender);
    if (adapter) {
        adapter->stopCommandTimer();
    }

    bool connected = false;
    if (!isNullReply(context, replyPtr)
            && !isEmptyReply(context, replyPtr))
    {
        auto reply = static_cast<redisReply *>(replyPtr);
        auto pongMessage = QString(reply->str);
        if (pongMessage == PING_REPLY) {
            connected = true;
        }
    }
    if (connected) {
        reDebug() << metaInfo(context).c_str() << "ping ok";
    } else {
        reDebug() << metaInfo(context).c_str() << "ping error: no connection";
    }
    if (adapter) {
        adapter->setConnected(connected);
    }
}

void Connector::selectCallback(redisReply *reply)
{
    if (isNullReply(reply) || isEmptyReply(reply))
    {
        return;
    }
    reDebug() << metaInfo().c_str() << "select status:" << toString(reply);
}

void Connector::connectCallback(const redisAsyncContext *context, int status)
{
    auto adapter = static_cast<Connector *>(context->data);
    reDebug() << metaInfo(context).c_str() << "Connected with status" << status;
    if (adapter->isBlocked()) {
        return;
    }
    if (status != REDIS_OK) {
        // hiredis already freed the context
        adapter->nullifyContext();
    }
}

void Connector::disconnectCallback(const redisAsyncContext *context, int status)
{
    auto adapter = static_cast<Connector *>(context->data);
    reDebug() << metaInfo(context).c_str() << "Disconnected with status" << status;
    adapter->setConnected(false);
    // hiredis already freed the context
    adapter->nullifyContext();
}

bool Connector::isConnected() const
{
    return m_isConnected;
}

int Connector::runAsyncCommand(const QString &command)
{
    if (!isConnected() || !isValidContext(m_redisContext)) {
        return REDIS_ERR;
    }
    auto status = redisAsyncCommand(m_redisContext, nullptr, nullptr, command.toStdString().c_str());
    return status;
}

int Connector::runAsyncCommand(StaticCb callback, const QString &command, void *data)
{
    return runAsyncCommandImplementation<CallbackArgsPlain>(
           privateCallbackStatic,
           command,
           connAlloc<CallbackArgsPlain>(callback, data));
}

void Connector::privateCallbackStatic(redisAsyncContext *context, void *replyPtr, void *data)
{
    auto args = static_cast<CallbackArgsPlain*>(data);
    auto sender = static_cast<Connector*>(context->data);
    args->callback(context, replyPtr, context->data);
    sender->finishAsyncCommand();
    connDealloc(args);
}

bool Connector::isNullReply(redisAsyncContext *context, void *replyPtr)
{
    auto reply = static_cast<redisReply *>(replyPtr);
    if (reply == nullptr) {
        if (context) {
            reDebug() << metaInfo(context).c_str() << "Error: null reply"
                      << context->err << context->errstr;
        } else {
            reDebug() << "Error: null reply. No context found.";
        }
        return true;
    }
    return false;
}

bool Connector::isEmptyReply(redisAsyncContext *context, void *replyPtr)
{
    auto reply = static_cast<redisReply *>(replyPtr);
    if (QString(reply->str).isEmpty()) {
        if (isValidContext(context)) {
            reDebug() << metaInfo(context).c_str() << "Error: reply string is empty";
        } else {
            reDebug() << "Error: reply string is empty";
        }
        return true;
    }
    return false;
}

void Connector::setConnected(bool state)
{
    if (m_isConnected != state) {
        m_isConnected = state;

        if (m_isConnected) {
            reInfo() << metaInfo().c_str() << "Connection successful.";
            emit connected();
            resetErrorCounter();
        } else {
            reError() << metaInfo().c_str() << "Lost connection.";
            emit disconnected();
        }
    }
}

bool Connector::isBlocked() const
{
    return m_reconnectCooldown->isActive();
}

void Connector::enablePingKeepalive()
{
    if (!m_pingConnection)
        m_pingConnection = m_pingTimer->callOnTimeout(this, &Connector::doPing);
    connect(this, &Connector::connected, m_pingTimer, QOverload<>::of(&QTimer::start));
    connect(this, &Connector::disconnected, m_pingTimer, &QTimer::stop);
}

void Connector::disablePingKeepalive()
{
    disconnect(m_pingTimer);
    m_pingTimer->disconnect();
    m_pingConnection = {};
}

void Connector::allowSelectDb()
{
    m_canSelect = true;
}

void Connector::blockSelectDb()
{
    m_canSelect = false;
}


bool Connector::isValidContext()
{
    return (context() != nullptr) && !(context()->err);
}

bool Connector::isNullReply(redisReply *reply)
{
    if (reply == nullptr) {
        if (context()) {
            reDebug() << metaInfo().c_str() << "Error: null reply"
                      << context()->err << context()->errstr;
        } else {
            reDebug() << "Error: null reply. No context found.";
        }
        return true;
    }
    return false;
}

bool Connector::isEmptyReply(redisReply *reply)
{
    if (QString(reply->str).isEmpty()) {
        if (isValidContext(context())) {
            reDebug() << metaInfo().c_str() << "Error: reply string is empty";
        } else {
            reDebug() << "Error: reply string is empty";
        }
        return true;
    }
    return false;
}

bool Connector::isValidContext(const redisAsyncContext *context)
{
    return (context != nullptr) && !(context->err);
}

QString Connector::host() const
{
    return m_host;
}

quint16 Connector::port() const
{
    return m_port;
}

quint16 Connector::dbIndex() const
{
    return m_dbIndex;
}

void Connector::setDbIndex(const quint16 dbIndex)
{
    if (m_dbIndex != dbIndex) {
        m_dbIndex = dbIndex;
        selectDb();
    }
}

quint16 Connector::port(const redisAsyncContext *context)
{
    if (!isValidContext(context)) {
        return 0u;
    }
    auto adapter = static_cast<Connector *>(context->data);
    auto redisPort = adapter ? adapter->port() : static_cast<quint16>(context->c.tcp.port);
    return redisPort;
}

std::string Connector::metaInfo(const redisAsyncContext *context, const int connectionPort, const QString &id)
{
    auto serverPort = connectionPort < 0 ? port(context) : connectionPort;
    auto info = QString("[ %1 ]").arg(serverPort);
    auto idString = id;
    if (idString.isEmpty() && isValidContext(context)) {
        auto adapter = static_cast<Connector *>(context->data);
        if (adapter) {
            idString = QStringLiteral("%1: %2 |").arg(adapter->workerName(), adapter->id());
        }
    }
    if (!idString.isEmpty()) {
        info += " " + idString;
    }
    return info.toStdString();
}

std::string Connector::metaInfo() const
{
    auto infoString = metaInfo(m_redisContext, port(), id());
    return infoString;
}

void *Connector::enqueueMsg(const Radapter::WorkerMsg &msg)
{
    return new quint64(m_queue.insert(msg.id(), msg).key());
}

Radapter::WorkerMsg Connector::dequeueMsg(void* msgId)
{
    if (!msgId) return {};
    auto key = static_cast<quint64*>(msgId);
    auto result = m_queue.take(*key);
    delete key;
    return result;
}

void Connector::disposeId(void *msgId)
{
    if (!msgId) return;
    auto key = static_cast<quint64*>(msgId);
    m_queue.remove(*key);
    delete key;
}

QString Connector::id() const
{
    return metaObject()->className();
}

QString Connector::toString(const redisReply *reply)
{
    if (!reply) {
        return QString{};
    }
    if (reply->type == REDIS_REPLY_NIL) {
        return QString{};
    }
    auto replyString = QString{};
    switch (reply->type) {
    case REDIS_REPLY_STRING:
    case REDIS_REPLY_STATUS:
    case REDIS_REPLY_ERROR: replyString = QString(reply->str); break;
    case REDIS_REPLY_INTEGER: replyString = QString::number(reply->integer); break;
    case REDIS_REPLY_DOUBLE: replyString = QString::number(reply->dval); break;
    default: break;
    }
    return replyString;
}
