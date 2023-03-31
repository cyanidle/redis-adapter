#include "redisconnector.h"
#include "radapterlogging.h"
#ifdef _MSC_VER
#include <WinSock2.h>
#endif
#include <QDataStream>

using namespace Redis;

Connector::Connector(const Settings::RedisConnector &settings, QThread *thread) :
    Worker(settings.worker, thread),
    m_config(settings),
    m_reconnectTimer(new QTimer(this)),
    m_commandTimeout(new QTimer(this))
{
    m_reconnectTimer->setSingleShot(true);
    m_commandTimeout->setSingleShot(true);
    m_reconnectTimer->callOnTimeout(this, &Connector::reconnect);
    m_commandTimeout->callOnTimeout(this, &Connector::onCommandTimeout);
    connect(this, &Connector::connected, &Connector::selectDb);
    connect(this, &Connector::connected, m_reconnectTimer, &QTimer::stop);
    connect(this, &Connector::disconnected, m_reconnectTimer, QOverload<>::of(&QTimer::start));
    m_reconnectTimer->setInterval(m_config.reconnect_delay);
    m_commandTimeout->setInterval(m_config.command_timeout);
    enablePingKeepalive();
}

Connector::~Connector()
{
    if (m_isConnected) {
        m_redisContext->data = nullptr;
        redisAsyncDisconnect(m_redisContext);
    } else if (m_redisContext) {
        redisAsyncFree(m_redisContext);
    }
}

void Connector::reconnect()
{
    workerError(this) << "Connection error or timeout. Trying new connection...";
    clearContext();
    tryConnect();
}


void Connector::tryConnect()
{
    if (isConnected()) {
        workerError(this) << "Attempt to connect while connected";
        return;
    }
    timeval timeout{0, m_config.tcp_timeout};
    auto options = redisOptions{};
    REDIS_OPTIONS_SET_TCP(&options, m_config.server.host->toStdString().c_str(), m_config.server.port);
    options.connect_timeout = &timeout;
    m_redisContext = redisAsyncConnectWithOptions(&options);
    m_redisContext->data = this;
    m_client->setContext(m_redisContext);
    redisAsyncSetConnectCallback(m_redisContext, connectCallback);
    redisAsyncSetDisconnectCallback(m_redisContext, disconnectCallback);
    if (m_redisContext->err) {
        workerError(this) << "Connection error:" << m_redisContext->errstr;
        clearContext();
        m_reconnectTimer->start();
    }
}

void Connector::clearContext()
{
    if (m_redisContext) {
        redisAsyncFree(m_redisContext);
        m_redisContext = nullptr;
    }
}

void Connector::onCommandTimeout()
{
    if (++m_commandTimeoutsCounter >= m_config.max_command_errors) {
        m_commandTimeoutsCounter = m_config.max_command_errors;
        setConnected(false);
    }
}

void Connector::selectDb()
{
    auto selectCommand = QStringLiteral("SELECT %2").arg(m_config.db_index);
    runAsyncCommand(&Connector::selectCallback, selectCommand);
}

void Connector::doPing()
{
    runAsyncCommand(&Connector::pingCallback, "PING");
}

void Connector::pingCallback(redisReply *replyPtr)
{
    auto parsed = parseReply(replyPtr);
    bool connected = parsed.toString() == "PONG" ||
                     parsed.toStringList().startsWith("PONG");
    setConnected(connected, "Ping error");
}

void Connector::startAsyncCommand(bool bypassTrack)
{
    if (m_pingTimer) {
        m_pingTimer->stop();
    }
    if (!bypassTrack) {
        m_commandTimeout->start();
    }
}

void Connector::finishAsyncCommand()
{
    setConnected(true);
    if (!commandsLeft()) {
        emit commandsFinished();
    }
}

void Connector::selectCallback(redisReply *reply)
{
    workerInfo(this) << "select status:" << parseReply(reply).toString();
}

void Connector::connectCallback(const redisAsyncContext *context, int status)
{
    auto adapter = static_cast<Connector *>(context->data);
    workerInfo(adapter) << "Connected with status" << status;
    if (status != REDIS_OK) {
        // hiredis already freed the context
        adapter->m_redisContext = nullptr;
        adapter->m_reconnectTimer->start();
    } else {
        adapter->setConnected(true);
    }
}

QVariantMap Connector::parseHashReply(redisReply *reply) const
{
    auto subresult = parseReply(reply).toList();
    QVariantMap result;
    for (int i = 1; i < subresult.size(); i+=2) {
        result.insert(subresult[i-1].toString(), subresult[i]);
    }
    return result;
}

QVariant Connector::parseReply(redisReply *reply) const
{
    if (!reply) {
        return {};
    }
    auto array = QVariantList{};
    switch (reply->type) {
    case ReplyString:
        return reply->str;
    case ReplyArray:
        for (size_t i = 0; i < reply->elements; ++i) {
            array.append(parseReply(reply->element[i]));
        }
        return array;
    case ReplyInteger:
        return reply->integer;
    case ReplyNil:
        return {};
    case ReplyStatus:
        return reply->str;
    case ReplyError:
        workerError(this) << "Received Error Reply:" << reply->str;
        return {};
    case ReplyDouble:
        return reply->dval;
    case ReplyBool:
        return bool(reply->integer);
    case ReplyMap:
        throw std::runtime_error("REDIS_REPLY_MAP Unsupported");
    case ReplySet:
        for (size_t i = 0; i < reply->elements; ++i) {
            array.append(parseReply(reply->element[i]));
        }
        return array;
    case ReplyAttr:
        throw std::runtime_error("REDIS_REPLY_ATTR Unsupported");
    case ReplyPush:
        throw std::runtime_error("REDIS_REPLY_PUSH Unsupported");
    case ReplyBignum:
        return reply->str;
    case ReplyVerb:
        return reply->str;
    default:
        return {};
    }
}

void Connector::disconnectCallback(const redisAsyncContext *context, int status)
{
    auto adapter = static_cast<Connector *>(context->data);
    workerInfo(adapter) << "Disconnected with status" << status;
    adapter->setConnected(false);
    adapter->m_redisContext = nullptr;
    adapter->setConnected(false);
}

bool Connector::isConnected() const
{
    return m_isConnected;
}

int Connector::runAsyncCommand(const QString &command)
{
    if (!isConnected() || !isValidContext()) {
        return REDIS_ERR;
    }
    return redisAsyncCommand(m_redisContext, nullptr, nullptr, command.toStdString().c_str());
}

int Connector::commandsLeft() const
{
    auto count = 0;
    auto current = context()->replies.head;
    while(current) {
        count++;
        current = current->next;
    }
    return count;
}

void Connector::setConnected(bool state, const QString &reason)
{
    if (m_isConnected != state) {
        m_isConnected = state;
        if (m_isConnected) {
            workerInfo(this) << "Connection successful.";
            emit connected();
        } else {
            workerError(this) << "Lost connection. Reason:" << (reason.isEmpty() ? "Not Given" : reason);;
            emit disconnected();
        }
    }
    if (state) {
        m_commandTimeout->stop();
        m_commandTimeoutsCounter = 0;
    }
}

void Connector::enablePingKeepalive()
{
    if (!m_pingTimer) {
        m_pingTimer = new QTimer(this);
        m_pingTimer->setInterval(m_config.ping_delay);
        m_pingTimer->callOnTimeout(this, &Connector::doPing);
        connect(this, &Connector::connected, m_pingTimer, QOverload<>::of(&QTimer::start));
        connect(this, &Connector::disconnected, m_pingTimer, &QTimer::stop);
        connect(this, &Connector::commandsFinished, m_pingTimer, QOverload<>::of(&QTimer::start));
    }
}

void Connector::disablePingKeepalive()
{
    if (m_pingTimer) {
        m_pingTimer->deleteLater();
        m_pingTimer = nullptr;
    }
}

bool Connector::isValidContext()
{
    return context() && !context()->err;
}

void Connector::setDbIndex(const quint16 dbIndex)
{
    if (m_config.db_index != dbIndex) {
        m_config.db_index = dbIndex;
        selectDb();
    }
}

void Connector::onRun()
{
    workerInfo(this, .noquote().nospace()) << ": Connnecting to: " << m_config.print() <<
                                    "(Host: " << m_config.server.host.value << "; Port: " << m_config.server.port.value << ")";
    m_client = new RedisQtAdapter(this);
    tryConnect();
    Radapter::Worker::onRun();
}
