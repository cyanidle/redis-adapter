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
#define RECONNECT_DELAY_MS      1000

using namespace Redis;

RedisConnector::RedisConnector(const QString &host,
                               const quint16 port,
                               const quint16 dbIndex,
                               const Radapter::WorkerSettings &settings)
    : WorkerBase(settings),
      m_connectionTimer(nullptr),
      m_pingTimer(nullptr),
      m_commandTimer(nullptr),
      m_reconnectCooldown(nullptr),
      m_isConnected(false),
      m_commandTimeoutsCounter{},
      m_commandStack{},
      m_redisContext(nullptr),
      m_client(nullptr),
      m_host(host),
      m_port(port),
      m_dbIndex(dbIndex),
      m_canSelect(true)
{
}

RedisConnector::~RedisConnector()
{
    if (m_isConnected) {
        redisAsyncDisconnect(m_redisContext);
    } else if (m_redisContext) {
        redisAsyncFree(m_redisContext);
    }
    if (m_client) {
        delete m_client;
    }
}

void RedisConnector::run()
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
    connect(this, &RedisConnector::connected, &RedisConnector::resetConnectionTimeout);
    connect(this, &RedisConnector::connected, &RedisConnector::stopConnectionTimer);

    m_reconnectCooldown = new QTimer(this);
    m_reconnectCooldown->setSingleShot(true);
    m_reconnectCooldown->setInterval(RECONNECT_DELAY_MS);
    m_reconnectCooldown->callOnTimeout(this, &RedisConnector::tryConnect);
    connect(this, &RedisConnector::disconnected, m_reconnectCooldown, QOverload<>::of(&QTimer::start));

    m_pingTimer = new QTimer(this);
    m_pingTimer->setSingleShot(false);
    m_pingTimer->setInterval(PING_TIMEOUT_MS);
    enablePingKeepalive();

    m_commandTimer = new QTimer(this);
    m_commandTimer->setSingleShot(true);
    m_commandTimer->setInterval(COMMAND_TIMEOUT_MS);
    m_commandTimer->callOnTimeout(this, [this](){
        if (m_commandTimeoutsCounter < MAX_COMMAND_ERRORS) {
            reDebug() << metaInfo().c_str() << "command timeout";
        }
        incrementErrorCounter();
    });

    connect(this, &RedisConnector::connected, &RedisConnector::selectDb);
    tryConnect();
}

void RedisConnector::finishAsyncCommand()
{
    if (!m_commandStack.isEmpty()) {
        m_commandStack.pop();
    }
    if (m_commandStack.isEmpty()) {
        m_pingTimer->start();
        emit commandsFinished();
    }
}

void RedisConnector::tryConnect()
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

void RedisConnector::doPing()
{
    redisAsyncCommand(m_redisContext, pingCallback, this, PING_REQUEST);
    m_commandTimer->start();
}

void RedisConnector::clearContext()
{
    if (isValidContext(m_redisContext)) {
        redisAsyncFree(m_redisContext);
    }
    nullifyContext();
}

void RedisConnector::nullifyContext()
{
    m_redisContext = nullptr;
}

void RedisConnector::resetConnectionTimeout()
{
    m_connectionTimer->setInterval(TCP_CONNECT_TIMEOUT_MS);
}

void RedisConnector::increaseConnectionTimeout()
{
    auto timeout = m_connectionTimer->interval() * 2;
    if (timeout > (TCP_CONNECT_TIMEOUT_MS * MAX_TIMEOUT_FACTOR)) {
        timeout = (TCP_CONNECT_TIMEOUT_MS * MAX_TIMEOUT_FACTOR);
    }
    m_connectionTimer->setInterval(timeout);
}

void RedisConnector::stopConnectionTimer()
{
    m_connectionTimer->stop();
}

void RedisConnector::incrementErrorCounter()
{
    m_commandTimeoutsCounter++;
    if (m_commandTimeoutsCounter >= MAX_COMMAND_ERRORS) {
        m_commandTimeoutsCounter = MAX_COMMAND_ERRORS;
        setConnected(false);
    }
}

void RedisConnector::resetErrorCounter()
{
    m_commandTimeoutsCounter = 0u;
}

void RedisConnector::stopCommandTimer()
{
    m_commandTimer->stop();
}

void RedisConnector::selectDb()
{
    if (!m_canSelect) {
        return;
    }
    auto selectCommand = QString("%1 %2").arg(SELECT_DB_REQUEST).arg(m_dbIndex);
    runAsyncCommand(selectCallback, selectCommand);
}

void RedisConnector::pingCallback(redisAsyncContext *context, void *replyPtr, void *sender)
{
    auto adapter = static_cast<RedisConnector *>(sender);
    if (adapter) {
        adapter->stopCommandTimer();
    }

    bool connected = false;
    if (!isNullReply(context, replyPtr, sender)
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

void RedisConnector::selectCallback(redisAsyncContext *context, void *replyPtr, void *sender)
{
    if (isNullReply(context, replyPtr, sender)
            || isEmptyReply(context, replyPtr))
    {
        return;
    }
    auto reply = static_cast<redisReply *>(replyPtr);
    reDebug() << metaInfo(context).c_str() << "select status:" << toString(reply);
    auto adapter = static_cast<RedisConnector *>(sender);
    adapter->finishAsyncCommand();
}

void RedisConnector::connectCallback(const redisAsyncContext *context, int status)
{
    auto adapter = static_cast<RedisConnector *>(context->data);
    reDebug() << metaInfo(context).c_str() << "Connected with status" << status;
    if (adapter->isBlocked()) {
        return;
    }
    if (status != REDIS_OK) {
        // hiredis already freed the context
        adapter->nullifyContext();
    }
}

void RedisConnector::disconnectCallback(const redisAsyncContext *context, int status)
{
    auto adapter = static_cast<RedisConnector *>(context->data);
    reDebug() << metaInfo(context).c_str() << "Disconnected with status" << status;
    adapter->setConnected(false);
    // hiredis already freed the context
    adapter->nullifyContext();
}

bool RedisConnector::isConnected() const
{
    return m_isConnected;
}

int RedisConnector::runAsyncCommand(const QString &command)
{
    if (!isConnected() || !isValidContext(m_redisContext)) {
        return REDIS_ERR;
    }
    auto status = redisAsyncCommand(m_redisContext, nullptr, nullptr, command.toStdString().c_str());
    return status;
}

int RedisConnector::runAsyncCommand(redisCallbackFn *callback, const QString &command, const QVariant &args)
{
    if (!isConnected() || !isValidContext(m_redisContext)) {
        return REDIS_ERR;
    }
    m_pingTimer->stop();
    void* cbArgs = this;
    if (args.isValid()) {
        cbArgs = new CallbackArgs{this, args};
    }
    auto status = redisAsyncCommand(m_redisContext, callback, cbArgs, command.toStdString().c_str());

    if (!m_commandStack.contains(callback)) {
        m_commandStack.push(callback);
    }
    return status;
}

bool RedisConnector::isNullReply(redisAsyncContext *context, void *replyPtr, void *sender)
{
    auto reply = static_cast<redisReply *>(replyPtr);
    auto adapter = static_cast<RedisConnector *>(sender);
    if (reply == nullptr || adapter == nullptr) {
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

bool RedisConnector::isEmptyReply(redisAsyncContext *context, void *replyPtr)
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

void RedisConnector::setConnected(bool state)
{
    if (m_isConnected != state) {
        m_isConnected = state;

        if (m_isConnected) {
            reInfo() << metaInfo().c_str() << "Connection successful.";
            emit connected();
        } else {
            reError() << metaInfo().c_str() << "Lost connection.";
            emit disconnected();
        }
    }
}

bool RedisConnector::isBlocked() const
{
    return m_reconnectCooldown->isActive();
}

void RedisConnector::enablePingKeepalive()
{
    m_pingTimer->callOnTimeout(this, &RedisConnector::doPing);
    connect(this, &RedisConnector::connected, m_pingTimer, QOverload<>::of(&QTimer::start));
    connect(this, &RedisConnector::disconnected, m_pingTimer, &QTimer::stop);
}

void RedisConnector::disablePingKeepalive()
{
    disconnect(m_pingTimer);
    m_pingTimer->disconnect();
}

void RedisConnector::allowSelectDb()
{
    m_canSelect = true;
}

void RedisConnector::blockSelectDb()
{
    m_canSelect = false;
}

bool RedisConnector::isValidContext(const redisAsyncContext *context)
{
    return (context != nullptr) && !(context->err);
}

QString RedisConnector::host() const
{
    return m_host;
}

quint16 RedisConnector::port() const
{
    return m_port;
}

quint16 RedisConnector::dbIndex() const
{
    return m_dbIndex;
}

void RedisConnector::setDbIndex(const quint16 dbIndex)
{
    if (m_dbIndex != dbIndex) {
        m_dbIndex = dbIndex;
        selectDb();
    }
}

quint16 RedisConnector::port(const redisAsyncContext *context)
{
    if (!isValidContext(context)) {
        return 0u;
    }
    auto adapter = static_cast<RedisConnector *>(context->data);
    auto redisPort = adapter ? adapter->port() : static_cast<quint16>(context->c.tcp.port);
    return redisPort;
}

std::string RedisConnector::metaInfo(const redisAsyncContext *context, const int connectionPort, const QString &id)
{
    auto serverPort = connectionPort < 0 ? port(context) : connectionPort;
    auto info = QString("[ %1 ]").arg(serverPort);
    auto idString = id;
    if (idString.isEmpty() && isValidContext(context)) {
        auto adapter = static_cast<RedisConnector *>(context->data);
        if (adapter) {
            idString = adapter->id();
        }
    }
    if (!idString.isEmpty()) {
        info += QString(" %1 :").arg(idString);
    }
    return info.toStdString();
}

std::string RedisConnector::metaInfo() const
{
    auto infoString = metaInfo(m_redisContext, port(), id());
    return infoString;
}


QString RedisConnector::id() const
{
    auto idString = QString(metaObject()->className());
    return idString;
}

QString RedisConnector::toString(const redisReply *reply)
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
