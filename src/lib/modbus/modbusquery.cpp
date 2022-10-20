#include "modbusquery.h"
#include "logging.h"

#define MODBUS_POLL_TIME_MS   500
#define MODBUS_EXEC_DELAY_MS  50

using namespace Modbus;

ModbusQuery::ModbusQuery(QModbusClient *client,
                         const RequestType type,
                         const QModbusDataUnit &query,
                         const quint8 serverAddress,
                         const quint16 pollRate,
                         QObject *parent)
    : QObject(parent),
      m_client(client),
      m_type(type),
      m_query(query),
      m_serverAddress(serverAddress),
      m_pollMode{},
      m_pollRate(pollRate > 0u ? pollRate : MODBUS_POLL_TIME_MS),
      m_pollTimer(nullptr),
      m_queryDoneOnce(false),
      m_quit(false),
      m_isFinished(true),
      m_isOk(false)
{
    m_responseTimer = new QTimer(this);
    m_responseTimer->setSingleShot(true);
    m_responseTimer->setInterval(m_client->timeout());
    connect(m_responseTimer, &QTimer::timeout, this, &ModbusQuery::emitResponseTimeout);

    m_execDelayTimer = new QTimer(this);
    m_execDelayTimer->setSingleShot(true);
    m_execDelayTimer->setInterval(pollRate > 0u ? pollRate : MODBUS_EXEC_DELAY_MS);
    m_execDelayTimer->callOnTimeout(this, &ModbusQuery::execOnce);
}

ModbusQuery::~ModbusQuery()
{
    if (m_pollTimer) {
        m_pollTimer->stop();
        delete m_pollTimer;
        m_pollTimer = nullptr;
    }
    m_client = nullptr;
}

void ModbusQuery::doRun()
{
    QModbusReply* reply = nullptr;
    if (m_type == RequestType::ModbusQueryTypeRead) {
        reply = m_client->sendReadRequest(m_query, m_serverAddress);
    } else if (m_type == RequestType::ModbusQueryTypeWrite) {
        reply = m_client->sendWriteRequest(m_query, m_serverAddress);
    } else {
        // no such type of request
    }

    if (reply) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &ModbusQuery::replyReady);
            connect(reply, &QModbusReply::errorOccurred, this, &ModbusQuery::captureReplyError);
            connect(this, &ModbusQuery::finished, reply, &QModbusReply::deleteLater);
            setIsFinished(false);
            m_responseTimer->start();
        } else {
            delete reply; // broadcast replies return immediately
        }
    } else {
        emitError(Q_FUNC_INFO + m_client->errorString());
    }
}

void ModbusQuery::captureReplyError()
{
    setHasSucceeded(false);
    auto reply = qobject_cast<QModbusReply*>(sender());
    m_responseTimer->stop();
    emit receivedError(slaveAddress(), Q_FUNC_INFO + reply->errorString());
}

void ModbusQuery::emitError(const QString errorString)
{
    setHasSucceeded(false);
    emit receivedError(slaveAddress(), errorString);
}

void ModbusQuery::setIsFinished(bool isFinished)
{
    m_isFinished = isFinished;
    if (m_isFinished) {
        emit finished();
    }
}

void ModbusQuery::setHasSucceeded(bool state)
{
    if (m_isOk != state) {
        m_isOk = state;
    }
}

void ModbusQuery::startPoll()
{
    m_pollMode = PollMode::Permanent;
    m_pollTimer = new QTimer(this);
    m_pollTimer->callOnTimeout(this, &ModbusQuery::doRun);
    m_pollTimer->setInterval(m_pollRate);
    m_pollTimer->start();
}

void ModbusQuery::execOnce()
{
    m_pollMode = PollMode::Single;
    doRun();
}

void ModbusQuery::execOnceDelayed()
{
    if (!m_execDelayTimer->isActive() && isFinished()) {
        m_execDelayTimer->start();
    }
}

bool ModbusQuery::isFinished() const
{
    return m_isFinished;
}

quint8 ModbusQuery::slaveAddress() const
{
    return m_serverAddress;
}

QModbusDataUnit ModbusQuery::request() const
{
    return m_query;
}

ModbusQuery::RequestType ModbusQuery::requestType() const
{
    return m_type;
}

quint16 ModbusQuery::pollRate() const
{
    return m_pollRate;
}

bool ModbusQuery::hasSucceeded() const
{
    return m_isOk;
}

bool ModbusQuery::doneOnce() const
{
    return m_queryDoneOnce;
}

void ModbusQuery::replyReady()
{
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) // what kind of error?
        return;

    m_responseTimer->stop();
    if (reply->error() == QModbusDevice::NoError) {
        setHasSucceeded(true);
        emit receivedReply(slaveAddress(), reply->result());
    } else {
        emitError(Q_FUNC_INFO + reply->errorString()); // any error handling needed?
    }
    setIsFinished(true);

    if (!m_queryDoneOnce && hasSucceeded()) {
        m_queryDoneOnce = true;
    }
}

void ModbusQuery::start()
{
    m_pollTimer->start();
}

void ModbusQuery::pause()
{
    m_pollTimer->stop();
}

void ModbusQuery::stop()
{
    setIsFinished(true);
}

void ModbusQuery::emitResponseTimeout()
{
    emitError("response timeout");
}

ModbusReadQuery::ModbusReadQuery(QModbusClient *client,
                                 const QModbusDataUnit &query,
                                 const quint8 serverAddress,
                                 const quint16 pollRate,
                                 QObject *parent)
    : ModbusQuery(client, RequestType::ModbusQueryTypeRead, query, serverAddress, pollRate, parent)
{
}

ModbusWriteQuery::ModbusWriteQuery(QModbusClient *client,
                                   const QModbusDataUnit &query,
                                   const quint8 serverAddress,
                                   const quint16 pollRate,
                                   QObject *parent)
    : ModbusQuery(client, RequestType::ModbusQueryTypeWrite, query, serverAddress, pollRate, parent)
{
}
