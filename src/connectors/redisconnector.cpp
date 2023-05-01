#include "redisconnector.h"
#include <QThread>
#include "radapterlogging.h"
#ifdef _MSC_VER
#include <WinSock2.h>
#endif
#include <QDataStream>
#include "settings/redissettings.h"
#include <QObject>
#include <QTimer>

using namespace Redis;

struct Connector::Private {
    Settings::RedisConnector config;
    redisAsyncContext* redisContext{};
    QTimer* reconnectTimer{};
    QTimer* commandTimeout{};
    RedisQtAdapter* client{};
    QTimer* pingTimer{nullptr};
    std::atomic<bool> isConnected{false};
    quint8 commandTimeoutsCounter{0};
};

Connector::Connector(const Settings::RedisConnector &settings, QThread *thread) :
    Worker(settings.worker, thread),
    d(new Private{settings})
{
    d->reconnectTimer = new QTimer(this);
    d->commandTimeout = new QTimer(this);
    d->reconnectTimer->setSingleShot(true);
    d->commandTimeout->setSingleShot(true);
    d->reconnectTimer->callOnTimeout(this, &Connector::reconnect);
    d->commandTimeout->callOnTimeout(this, &Connector::onCommandTimeout);
    connect(this, &Connector::connected, &Connector::selectDb);
    connect(this, &Connector::connected, d->reconnectTimer, &QTimer::stop);
    connect(this, &Connector::disconnected, d->reconnectTimer, QOverload<>::of(&QTimer::start));
    d->reconnectTimer->setInterval(d->config.reconnect_delay);
    d->commandTimeout->setInterval(d->config.command_timeout);
    enablePingKeepalive();
}

Connector::~Connector()
{
    if (d->isConnected) {
        d->redisContext->data = nullptr;
        redisAsyncDisconnect(d->redisContext);
    } else if (d->redisContext) {
        redisAsyncFree(d->redisContext);
    }
    delete d;
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
    timeval timeout{0, d->config.tcp_timeout};
    auto options = redisOptions{};
    REDIS_OPTIONS_SET_TCP(&options, d->config.server.host->toStdString().c_str(), d->config.server.port);
    options.connect_timeout = &timeout;
    d->redisContext = redisAsyncConnectWithOptions(&options);
    d->redisContext->data = this;
    d->client->setContext(d->redisContext);
    redisAsyncSetConnectCallback(d->redisContext, connectCallback);
    redisAsyncSetDisconnectCallback(d->redisContext, disconnectCallback);
    if (d->redisContext->err) {
        workerError(this) << "Connection error:" << d->redisContext->errstr;
        clearContext();
        d->reconnectTimer->start();
    }
}

void Connector::clearContext()
{
    if (d->redisContext) {
        redisAsyncFree(d->redisContext);
        d->redisContext = nullptr;
    }
}

void Connector::onCommandTimeout()
{
    if (++d->commandTimeoutsCounter >= d->config.max_command_errors) {
        d->commandTimeoutsCounter = d->config.max_command_errors;
        setConnected(false);
    }
}

void Connector::selectDb()
{
    auto selectCommand = QStringLiteral("SELECT %2").arg(d->config.db_index);
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
    if (d->pingTimer) {
        d->pingTimer->stop();
    }
    if (!bypassTrack) {
        d->commandTimeout->start();
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
        adapter->d->redisContext = nullptr;
        adapter->d->reconnectTimer->start();
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
        return QString(reply->str);
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
        return QString(reply->str);
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
        return QString(reply->str);
    case ReplyVerb:
        return QString(reply->str);
    default:
        return {};
    }
}

void Connector::disconnectCallback(const redisAsyncContext *context, int status)
{
    auto adapter = static_cast<Connector *>(context->data);
    workerInfo(adapter) << "Disconnected with status" << status;
    adapter->setConnected(false);
    adapter->d->redisContext = nullptr;
    adapter->setConnected(false);
}

bool Connector::isConnected() const
{
    return d->isConnected;
}

void Connector::waitConnected(Radapter::Worker *who) const
{
    auto name = who ? who->printSelf() : QStringLiteral("Unknown");
    workerInfo(this) << name << " is waiting for connection...";
    while (!isConnected()) {
        QThread::usleep(50);
    }
}

int Connector::runAsyncCommand(const QString &command)
{
    if (!isConnected() || !isValidContext()) {
        return REDIS_ERR;
    }
    return redisAsyncCommand(d->redisContext, nullptr, nullptr, command.toStdString().c_str());
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
    if (d->isConnected != state) {
        d->isConnected = state;
        if (d->isConnected) {
            workerInfo(this) << "Connection successful.";
            emit connected();
        } else {
            workerError(this) << "Lost connection. Reason:" << (reason.isEmpty() ? "Not Given" : reason);;
            emit disconnected();
        }
    }
    if (state) {
        d->commandTimeout->stop();
        d->commandTimeoutsCounter = 0;
    }
}

void Connector::enablePingKeepalive()
{
    if (!d->pingTimer) {
        d->pingTimer = new QTimer(this);
        d->pingTimer->setInterval(d->config.ping_delay);
        d->pingTimer->callOnTimeout(this, &Connector::doPing);
        connect(this, &Connector::connected, d->pingTimer, QOverload<>::of(&QTimer::start));
        connect(this, &Connector::disconnected, d->pingTimer, &QTimer::stop);
        connect(this, &Connector::commandsFinished, d->pingTimer, QOverload<>::of(&QTimer::start));
    }
}

void Connector::disablePingKeepalive()
{
    if (d->pingTimer) {
        d->pingTimer->deleteLater();
        d->pingTimer = nullptr;
    }
}

redisAsyncContext *Connector::context()
{
    return d->redisContext;
}

const redisAsyncContext *Connector::context() const
{
    return d->redisContext;
}

bool Connector::isValidContext()
{
    return context() && !context()->err;
}

void Connector::setDbIndex(const quint16 dbIndex)
{
    if (d->config.db_index != dbIndex) {
        d->config.db_index = dbIndex;
        selectDb();
    }
}

void Connector::onRun()
{
    workerInfo(this, .noquote().nospace()) << ": Connnecting to: " << d->config.print() <<
                                    "(Host: " << d->config.server.host.value << "; Port: " << d->config.server.port.value << ")";
    d->client = new RedisQtAdapter(this);
    tryConnect();
    Radapter::Worker::onRun();
}
