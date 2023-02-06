#include "redisconnector.h"
#include "broker/interceptors/logginginterceptor.h"
#include "radapterlogging.h"
#ifdef _MSC_VER
#include <WinSock2.h>
#endif
#include "utils/timeutils.h"
#include <QDataStream>

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

Connector::Connector(const Settings::RedisConnector &settings, QThread *thread) :
    Worker(settings.worker, thread),
    m_pingTimer(new QTimer(this)),
    m_redisContext(nullptr),
    m_reconnectTimer(new QTimer(this)),
    m_commandTimer(nullptr),
    m_isConnected(false),
    m_commandTimeoutsCounter{},
    m_client(nullptr),
    m_host(settings.server.host),
    m_port(settings.server.port),
    m_dbIndex(settings.db_index),
    m_canSelect(true)
{
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->callOnTimeout(this, &Connector::reconnect);
    resetReconnectTimeout();
    connect(this, &Connector::connected, &Connector::resetReconnectTimeout);
    connect(this, &Connector::connected, &Connector::stopReconnectTimer);
    connect(this, &Connector::disconnected, m_reconnectTimer, QOverload<>::of(&QTimer::start));
    m_pingTimer->setSingleShot(false);
    m_pingTimer->setInterval(PING_TIMEOUT_MS);
    enablePingKeepalive();
    m_commandTimer = new QTimer(this);
    m_commandTimer->setSingleShot(true);
    m_commandTimer->callOnTimeout(this, &Connector::registerCommandTimeout);
    resetCommandTimeout();
    connect(this, &Connector::connected, &Connector::selectDb);
    connect(this, &Connector::alive, &Connector::resetErrorCounter);
    if (settings.log_jsons) {
        addInterceptor(new Radapter::LoggingInterceptor(*settings.log_jsons));
    }
}

Connector::~Connector()
{
    if (m_isConnected) {
        m_redisContext->data = nullptr;
        redisAsyncDisconnect(m_redisContext);
    } else if (m_redisContext) {
        redisAsyncFree(m_redisContext);
        nullifyContext();
    }
}

void Connector::finishAsyncCommand()
{
    confirmAlive();
    if (m_pendingCommandsCounter && !--m_pendingCommandsCounter) {
        m_pingTimer->start();
        emit commandsFinished();
    }
}

void Connector::reconnect()
{
    reError() << metaInfo()  << "Connection error or timeout. Trying new connection...";
    increaseReconnectTimeout();
    clearContext();
    tryConnect();
}

void Connector::startCommandTimer()
{
    m_commandTimer->start();
}

void Connector::tryConnect()
{
    static timeval timeout{.tv_sec = 0, .tv_usec = TCP_CONNECT_TIMEOUT_MS};
    if (isConnected()) {
        return;
    }
    auto options = redisOptions{};
    REDIS_OPTIONS_SET_TCP(&options, m_host.toStdString().c_str(), m_port);
    options.connect_timeout = &timeout;
    m_redisContext = redisAsyncConnectWithOptions(&options);
    m_redisContext->data = this;
    m_client->setContext(m_redisContext);
    redisAsyncSetConnectCallback(m_redisContext, connectCallback);
    redisAsyncSetDisconnectCallback(m_redisContext, disconnectCallback);
    startReconnectTimer();
    if (m_redisContext->err) {
        reDebug() << metaInfo()  << "Error: " << m_redisContext->errstr;
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
        m_redisContext = nullptr;
    }
}

void Connector::resetReconnectTimeout()
{
    m_reconnectTimer->setInterval(TCP_CONNECT_TIMEOUT_MS);
}

void Connector::increaseReconnectTimeout()
{
    auto timeout = m_reconnectTimer->interval() * 2;
    if (timeout > (TCP_CONNECT_TIMEOUT_MS * MAX_TIMEOUT_FACTOR)) {
        timeout = (TCP_CONNECT_TIMEOUT_MS * MAX_TIMEOUT_FACTOR);
    }
    m_reconnectTimer->setInterval(timeout);
}

void Connector::startReconnectTimer()
{
    m_reconnectTimer->start();
}

void Connector::stopReconnectTimer()
{
    m_reconnectTimer->stop();
}

void Connector::registerCommandTimeout()
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
    auto reply = static_cast<redisReply *>(replyPtr);
    if (adapter) {
        adapter->stopCommandTimer();
    }
    bool connected = false;
    if (isValidReply(reply))
    {
        // pubsub mode generates reply array
        auto pongMessage = (reply->type == REDIS_REPLY_ARRAY)
                ? QString(reply->element[0]->str)
                : QString(reply->str);
        if (pongMessage.toUpper() == PING_REPLY) {
            connected = true;
        }
    }
    if (!connected) {
        reDebug() << metaInfo(context) << "ping error: no connection";
    }
    if (adapter) {
        adapter->setConnected(connected);
    }
}

void Connector::selectCallback(redisReply *reply)
{
    if (!isValidReply(reply))
    {
        return;
    }
    confirmAlive();
    reDebug() << metaInfo()  << "select status:" << toString(reply);
}

void Connector::connectCallback(const redisAsyncContext *context, int status)
{
    auto adapter = static_cast<Connector *>(context->data);
    reDebug() << metaInfo(context)  << "Connected with status" << status;
    if (status != REDIS_OK) {
        // hiredis already freed the context
        adapter->nullifyContext();
    }
}

void Connector::nullifyContext()
{
    m_redisContext = nullptr;
}

//auto entryId = QString(streamReply->element[0]->str);
//    if (entryId.isEmpty()) {
//        return;
//    }
//    auto itemData = streamReply->element[1];
//    if (itemData->elements == 0) {
//        return;
//    }

//    auto fields = JsonDict{};
//    for (quint16 i = 0; i < itemData->elements; i += 2) {
//        auto fieldKey = QString(itemData->element[i]->str);
//        auto fieldValue = QString(itemData->element[i + 1]->str);
//        if (!fieldValue.isEmpty()) {
//            fields.insert(fieldKey.split(":"), fieldValue);
//        }
//    }
//    if (!fields.isEmpty()) {
//        auto jsonEntry = JsonDict{};
//        jsonEntry.insert(entryId, fields);
//        m_streamEntry = jsonEntry;
//    }

QVariantMap Connector::parseHashReply(redisReply *reply)
{
    auto subresult = parseReply(reply).toList();
    QVariantMap result;
    for (int i = 1; i < subresult.size(); i+=2) {
        result.insert(subresult[i-1].toString(), subresult[i]);
    }
    return result;
}

QVariant Connector::parseReply(redisReply *reply)
{
    if (!isValidReply(reply)) {
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
        return reply->str;
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
    reDebug() << metaInfo(context)  << "Disconnected with status" << status;
    if (adapter) {
        adapter->setConnected(false);
        adapter->nullifyContext();
    }
}

void Connector::setCommandTimeout(int milliseconds)
{
    if (m_commandTimer->interval() != milliseconds) {
        m_commandTimer->setInterval(milliseconds);
    }
}

void Connector::resetCommandTimeout()
{
    setCommandTimeout(COMMAND_TIMEOUT_MS);
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
    return redisAsyncCommand(m_redisContext, nullptr, nullptr, command.toStdString().c_str());
}

int Connector::runAsyncCommand(StaticCb callback, const QString &command, void *data, bool needBypassTracking)
{
    return runAsyncCommandImplementation<CallbackArgsPlain>(
               privateCallbackStatic,
               command,
               connAlloc<CallbackArgsPlain>(callback, data),
               needBypassTracking);
}

void Connector::privateCallbackStatic(redisAsyncContext *context, void *replyPtr, void *data)
{
    auto args = static_cast<CallbackArgsPlain*>(data);
    auto sender = static_cast<Connector*>(context->data);
    args->callback(context, replyPtr, context->data);
    sender->finishAsyncCommand();
    connDealloc(args);
}

bool Connector::isValidReply(redisReply *reply)
{
    return reply;
}

void Connector::setConnected(bool state, const QString &reason)
{
    if (m_isConnected != state) {
        m_isConnected = state;
        if (m_isConnected) {
            reInfo() << metaInfo() << "Connection successful.";
            emit connected();
        } else {
            reError() << metaInfo() << "Lost connection.Reason:" <<
                         (reason.isEmpty() ? "Not Given" : reason);;
            emit disconnected();
        }
    }
}

void Connector::enablePingKeepalive()
{
    m_pingTimer->callOnTimeout(this, &Connector::doPing, Qt::UniqueConnection);
    connect(this, &Connector::connected, m_pingTimer, QOverload<>::of(&QTimer::start), Qt::UniqueConnection);
    connect(this, &Connector::disconnected, m_pingTimer, &QTimer::stop, Qt::UniqueConnection);
}

void Connector::disablePingKeepalive()
{
    if (m_pingTimer) {
        disconnect(m_pingTimer);
        m_pingTimer->disconnect();
    }
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
    return context() && !context()->err;
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

QString Connector::metaInfo(const redisAsyncContext *context, const int connectionPort, const QString &id)
{
    auto serverPort = connectionPort < 0 ? port(context) : connectionPort;
    auto info = QString("[ %1 ] { %2 }").arg(serverPort)
            .arg(toHex(QThread::currentThreadId()));
    auto idString = id;
    if (idString.isEmpty() && isValidContext(context)) {
        auto adapter = static_cast<Connector*>(context->data);
        if (adapter) {
            idString = adapter->id();
        }
    }
    if (!idString.isEmpty()) {
        info += " " + idString;
    }
    return info;
}

QString Connector::metaInfo() const
{
    auto infoString = metaInfo(m_redisContext, port(), id());
    return infoString;
}

QString Connector::id() const
{
    return QStringLiteral("%1 ( %2 )").arg(metaObject()->className(), toHex(this));
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

void Connector::confirmAlive()
{
    emit alive();
}

void Connector::onRun()
{
    m_client = new RedisQtAdapter(this);
    tryConnect();
    Radapter::Worker::onRun();
}


QString Connector::toHex(const quintptr &pointer)
{
    auto bytes = QByteArray{};
    QDataStream byteStream{ &bytes, QIODevice::WriteOnly };
    byteStream.setByteOrder(QDataStream::BigEndian);
    byteStream << pointer;
    // убираем лишние нулевые байты из начала
    auto byteString = bytes.right(4).toHex();
    return QStringLiteral("0x") + byteString;
}

QString Connector::toHex(const void *pointer)
{
    auto uintPointer = toUintPointer(pointer);
    auto hexString = toHex(uintPointer);
    return hexString;
}

quintptr Connector::toUintPointer(const void *pointer)
{
    return reinterpret_cast<quintptr>(pointer);
}

QString Connector::replyTypeToString(const int replyType)
{
    return QMetaEnum::fromType<ReplyTypes>().valueToKey(replyType);
}
