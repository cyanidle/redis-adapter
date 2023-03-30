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
    m_reconnectTimer->setInterval(settings.reconnect_timeout_ms);
    m_reconnectTimer->callOnTimeout(this, &Master::connectDevice);
    m_reconnectTimer->setSingleShot(true);
    m_readTimer->setInterval(settings.poll_rate);
    m_readTimer->callOnTimeout(this, &Master::doRead);
    for (auto regIter = settings.registers.cbegin(); regIter != settings.registers.cend(); ++regIter) {
        if (m_reverseRegisters[regIter->table].contains(regIter->index)) {
            throw std::invalid_argument("Register index collission on: " +
                                        regIter.key().toStdString() +
                                        "; With --> " +
                                        m_reverseRegisters[regIter->table][regIter->index].toStdString() +
                                        " (Table: "+ tableToString(regIter->table).toStdString() +
                                        "; Register: " + QString::number(regIter->index.value).toStdString() + ")");
        }
        m_reverseRegisters[regIter->table][regIter->index] = regIter.key();
    }
    connect(this, &Master::connected, [this](){m_connected=true;});
    connect(this, &Master::disconnected, [this](){m_connected=false;});
}

void Master::initClient()
{
    if (m_settings.device.tcp.value) {
        m_device = new QModbusTcpClient(this);
        m_device->setConnectionParameter(QModbusDevice::NetworkAddressParameter, m_settings.device.tcp->host.value);
        m_device->setConnectionParameter(QModbusDevice::NetworkPortParameter, m_settings.device.tcp->port.value);
    } else {
        m_device = new QModbusRtuSerialMaster(this);
        m_device->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, m_settings.device.rtu->baud);
        m_device->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, m_settings.device.rtu->data_bits);
        m_device->setConnectionParameter(QModbusDevice::SerialParityParameter, m_settings.device.rtu->parity);
        m_device->setConnectionParameter(QModbusDevice::SerialPortNameParameter, m_settings.device.rtu->port_name.value);
        m_device->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, m_settings.device.rtu->stop_bits);
    }
    m_device->setTimeout(m_settings.responce_time);
    m_device->setNumberOfRetries(m_settings.retries);
    connect(m_device, &QModbusDevice::stateChanged, this, &Master::onStateChanged);
    connect(m_device, &QModbusDevice::errorOccurred, this, &Master::onErrorOccurred);
}

void Master::onRun()
{
    initClient();
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
    auto channel = m_settings.device.channel.data();
    channel->registerUser(this);
    channel->callOnTrigger(this, &Master::executeNext);
    channel->signalJobDone(this, &Master::queryDone);
    channel->signalJobDone(this, &Master::allQueriesDone);
    channel->askTriggerOn(this, &Master::queryDone);
    channel->askTriggerOn(this, &Master::askTrigger);
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
                results.append(parseValueToDataUnit(value, regInfo));
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
}

void Master::reconnect() {
    workerWarn(this) << ": Reconnecting...";
    m_device->disconnectDevice();
    m_reconnectTimer->start();
}

void Master::onStateChanged(QModbusDevice::State state)
{
    if (state == QModbusDevice::UnconnectedState) {
        emit disconnected();
        m_reconnectTimer->start();
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
        workerError(this, .noquote()) << ": Error Reading:\n" << reply->errorString();
        reconnect();
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
        auto result = parseModbusType(words.data() + i, regData, sizeWords);
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
    auto unit = reply->result();
    if (reply->error() == QModbusDevice::NoError) {
        workerInfo(this, .nospace().noquote()) << ": Written:\n" << printUnit(unit);
    } else {
        workerError(this, .nospace().noquote()) << ": Error Writing:\n" << printUnit(unit) << ";\nReason: " << reply->errorString();
        reconnect();
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
        workerError(this) << "Read Error: " << m_device->errorString() << "; Reconnecting...";
        emit queryDone();
        reconnect();
    }
}

void Master::executeWrite(const QModbusDataUnit &unit)
{
    if (auto reply = m_device->sendWriteRequest(unit, m_settings.slave_id)) {
        workerInfo(this, .noquote()) << "Writing...:\n" << printUnit(unit);
        connect(reply, &QModbusReply::destroyed, this, &Master::queryDone);
        if (!reply->isFinished()) {
            QObject::connect(reply, &QModbusReply::finished, this, &Master::onWriteReady);
        } else {
            delete reply;
        }
    } else {
        workerInfo(this) << "Write Error: " << m_device->errorString() << "; Reconnecting...";
        emit queryDone();
        reconnect();
    }
}



