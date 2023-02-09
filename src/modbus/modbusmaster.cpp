#include "modbusmaster.h"
#include "broker/broker.h"
#include "templates/algorithms.hpp"
#include <QModbusRtuSerialMaster>
#include "modbusparsing.h"
#include <QModbusReply>
#include <QModbusTcpClient>

using namespace Modbus;
using namespace Radapter;

Master::Master(const Settings::ModbusMaster &settings, QThread *thread) :
    Radapter::Worker(settings.worker, thread),
    m_settings(settings),
    m_reconnectTimer(new QTimer(this)),
    m_readTimer(new QTimer(this))
{
    if (settings.log_jsons) {
        addInterceptor(new LoggingInterceptor(*settings.log_jsons));
    }

    m_reconnectTimer->setInterval(settings.reconnect_timeout_ms);
    m_reconnectTimer->callOnTimeout(this, &Master::connectDevice);
    m_reconnectTimer->setSingleShot(true);
    m_readTimer->setInterval(settings.poll_rate);
    m_readTimer->callOnTimeout(this, &Master::doRead);
    if (settings.device.tcp) {
        m_device = new QModbusTcpClient(this);
        m_device->setConnectionParameter(QModbusDevice::NetworkAddressParameter, settings.device.tcp->host);
        m_device->setConnectionParameter(QModbusDevice::NetworkPortParameter, settings.device.tcp->port);
    } else {
        m_device = new QModbusRtuSerialMaster(this);
        m_device->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, settings.device.rtu->baud);
        m_device->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, settings.device.rtu->data_bits);
        m_device->setConnectionParameter(QModbusDevice::SerialParityParameter, settings.device.rtu->parity);
        m_device->setConnectionParameter(QModbusDevice::SerialPortNameParameter, settings.device.rtu->port_name);
        m_device->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, settings.device.rtu->stop_bits);
    }
    m_device->setTimeout(settings.responce_time);
    m_device->setNumberOfRetries(settings.retries);
    for (auto regIter = settings.registers.cbegin(); regIter != settings.registers.cend(); ++regIter) {
        m_reverseRegisters[regIter->table][regIter->index] = regIter.key();
    }
    connect(m_device, &QModbusDevice::stateChanged, this, &Master::onStateChanged);
    connect(m_device, &QModbusDevice::errorOccurred, this, &Master::onErrorOccurred);
    connect(this, &Master::connected, [this](){m_connected=true;});
    connect(this, &Master::disconnected, [this](){m_connected=false;});
}

void Master::onRun()
{
    attachToChannel();
    connectDevice();
    if (config().poll_rate) {
        m_readTimer->start();
    }
    Worker::onRun();
}

Master::~Master()
{
    m_device->disconnectDevice();
}

bool Master::isConnected() const
{
    return m_connected;
}

void Master::attachToChannel()
{
    //! Creating channels only once per channel of all masters, then sharing them (QSharedPointer)
    static QMutex attachMut{};
    QMutexLocker lock(&attachMut);
    auto isSameChannel = [this](Master *master) {
        return master->config().channel == config().channel && master != this;
    };
    auto masters = broker()->getAll<Master>();
    auto sameChannel = filter(&masters, isSameChannel);
    if (sameChannel.nonePass()) {
        m_channel.reset(new Sync::Channel(workerThread()));
    } else {
        auto first = *sameChannel.begin();
        if (first->m_channel) {
            m_channel = first->m_channel;
        } else {
            m_channel.reset(new Sync::Channel(workerThread()));
        }
    }
    auto channel = m_channel.data();
    //! The main code
    channel->registerUser(this);
    channel->callOnTrigger(this, &Master::triggerExecute);
    channel->signalJobDone(this, &Master::queryDone);
    channel->signalJobDone(this, &Master::allQueriesDone);
    channel->askTriggerOn(this, &Master::queryDone);
    channel->askTriggerOn(this, &Master::askTrigger);
    connect(channel, &Sync::Channel::destroyed, this, &Master::onChannelDied);
}

const Settings::ModbusMaster &Master::config() const
{
    return m_settings;
}

void Master::onMsg(const Radapter::WorkerMsg &msg)
{
    if (!m_connected) {
        reDebug() << printSelf() << "Write while not connected!";
        return;
    }
    QList<QModbusDataUnit> results;
    for (auto& iter : msg) {
        auto fullKeyJoined = iter.key().join(":");
        if (m_settings.registers.contains(fullKeyJoined)) {
            auto regInfo = m_settings.registers.value(fullKeyJoined);
            auto value = iter.value();
            if (value.canConvert(regInfo.type)) {
                results.append(parseValueToDataUnit(value, regInfo, config().endianess));
            } else {
                reWarn() << printSelf() << ": Received: " << value << "; Key:" << fullKeyJoined;
            }
        }
    }
    for (auto &item : mergeDataUnits(results)) {
        enqeueWrite(item);
    }
}

void Master::connectDevice()
{
    if (!m_device->connectDevice()) {
        reDebug() << printSelf() << ": Error Connecting: " << m_device->errorString();
        m_reconnectTimer->start();
    }
}

void Master::disconnectDevice()
{
    m_device->disconnectDevice();
}

void Master::triggerExecute(QObject *target)
{
    if (target != this) return;
    executeNext();
}

void Master::executeNext()
{
    if (!m_writeQueue.isEmpty()) {
        executeWrite(m_writeQueue.dequeue());
    } else if (!m_readQueue.isEmpty()) {
        executeRead(m_readQueue.dequeue());
    } else {
        emit allQueriesDone();
    }
}

void Master::onErrorOccurred(QModbusDevice::Error error)
{
    workerError(this) << QMetaEnum::fromType<QModbusDevice::Error>().valueToKey(error) << "; Source: " << sender();
    disconnectDevice();
    m_reconnectTimer->start();
}

void Master::onStateChanged(QModbusDevice::State state)
{
    if (state == QModbusDevice::UnconnectedState) {
        m_reconnectTimer->start();
        emit disconnected();
    } else if (state == QModbusDevice::ConnectedState) {
        emit connected();
    }
    workerInfo(this) << "New State --> " << QMetaEnum::fromType<QModbusDevice::State>().valueToKey(state);
}

void Master::onReadReady()
{
    auto rawReply = qobject_cast<QModbusReply *>(sender());
    if (!rawReply) return;
    QScopedPointer<QModbusReply, QScopedPointerDeleteLater> reply{rawReply};
    if (reply->error() != QModbusDevice::NoError) {
        workerError(this) << ": Error Reading: " << reply->errorString();
        return;
    }
    auto words = reply->result().values();
    auto table = reply->result().registerType();
    JsonDict resultJson;
    for (int i = 0; i < words.size();) {
        if (!m_reverseRegisters[table].contains(reply->result().startAddress() + i)) {
            ++i;
            continue;
        }
        auto index = reply->result().startAddress() + i;
        const auto &registersName = m_reverseRegisters[table][index];
        const auto &regData = config().registers[registersName];
        auto sizeWords = QMetaType::sizeOf(regData.type)/2;
        if (i + sizeWords > words.size()) {
            break;
        }
        auto result = parseModbusType(words.data() + i, regData, sizeWords, config().endianess);
        i += sizeWords;
        resultJson.insert(registersName.split(':'), result);
    }
    formatAndSendJson(resultJson);
}

void Master::onWriteReady()
{
    auto rawReply = qobject_cast<QModbusReply *>(sender());
    if (!rawReply) return;
    QScopedPointer<QModbusReply, QScopedPointerDeleteLater> reply{rawReply};
    if (reply->error() == QModbusDevice::NoError) {
        workerError(this) << "; Written: " << reply->result().values();
    } else {
        workerError(this) << ": Error Writing: " << reply->errorString();
    }
}

void Master::doRead()
{
    if (!m_connected) {
        return;
    }
    for (auto &query : config().queries) {
        auto unit = QModbusDataUnit(query.type, query.reg_index, query.reg_count);
        enqeueRead(unit);
    }
}

void Master::onChannelDied(QObject *who)
{
    workerError(this, .nospace()) << "Channel (" << who << ") died, deleting...";
    deleteLater();
}

void Master::formatAndSendJson(const JsonDict &json)
{
    JsonDict result;
    for (auto &newJson : json) {
        auto key = newJson.key();
        auto lastVal = m_lastJson.value(key);
        if (lastVal != newJson.value()) {
            result.insert(key, newJson.value());
            m_lastJson[key] = newJson.value();
        }
    }
    if (!result.isEmpty()) {
        emit sendMsg(prepareMsg(result));
    }
}

void Master::enqeueRead(const QModbusDataUnit &unit)
{
    m_readQueue.enqueue(unit);
    emit askTrigger();
}

void Master::enqeueWrite(const QModbusDataUnit &unit)
{
    m_writeQueue.enqueue(unit);
    emit askTrigger();
}

void Master::executeRead(const QModbusDataUnit &unit)
{
    if (auto reply = m_device->sendReadRequest(unit, m_settings.slave_id)) {
        connect(reply, &QModbusReply::destroyed, this, &Master::queryDone);
        if (!reply->isFinished()) {
            QObject::connect(reply, &QModbusReply::finished, this, &Master::onReadReady);
        } else {
            delete reply;
        }
    } else {
        workerError(this) << "Read Error: " << m_device->errorString();
        emit queryDone();
    }
}

void Master::executeWrite(const QModbusDataUnit &unit)
{
    if (auto reply = m_device->sendWriteRequest(unit, m_settings.slave_id)) {
        connect(reply, &QModbusReply::destroyed, this, &Master::queryDone);
        if (!reply->isFinished()) {
            QObject::connect(reply, &QModbusReply::finished, this, &Master::onWriteReady);
        } else {
            delete reply;
        }
    } else {
        workerInfo(this) << "Write Error: " << m_device->errorString();
        emit queryDone();
    }
}



