#include "modbusmaster.h"
#include "utils/wordoperations.h"
#include <QModbusRtuSerialMaster>
#include <QModbusReply>
#include <QModbusTcpClient>

using namespace Modbus;
using namespace Radapter;
using namespace Utils;

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
    connectDevice();
    m_readTimer->start();
}

Master::~Master()
{
    m_device->disconnectDevice();
}

bool Master::isConnected() const
{
    return m_connected;
}

void Master::onMsg(const Radapter::WorkerMsg &msg)
{
    if (!m_connected) {
        reDebug() << "Write while not connected!";
        return;
    }
    QList<QModbusDataUnit> results;
    for (auto& iter : msg) {
        auto fullKeyJoined = iter.key().join(":");
        if (m_settings.registers.contains(fullKeyJoined)) {
            auto regInfo = m_settings.registers.value(fullKeyJoined);
            auto value = iter.value();
            if (Q_LIKELY(value.canConvert(regInfo.type))) {
                results.append(parseValueToDataUnit(value, regInfo, config().endianess));
            } else {
                reWarn() << "Incorrect value type for slave: " << workerName() << "; Received: " << value << "; Key:" << fullKeyJoined;
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
    } else {
        emit connected();
    }
}

void Master::disconnectDevice()
{
    m_device->disconnectDevice();
}

void Master::executeNext()
{
    if (!m_writeQueue.isEmpty()) {
        executeWrite(m_writeQueue.dequeue());
    } else if (!m_readQueue.isEmpty()) {
        executeRead(m_readQueue.dequeue());
    }
}

void Master::onErrorOccurred(QModbusDevice::Error error)
{
    reDebug() << printSelf() << ": Error: " << QMetaEnum::fromType<QModbusDevice::Error>().valueToKey(error) << "; Source: " << sender();
}

void Master::onStateChanged(QModbusDevice::State state)
{
    if (state == QModbusDevice::UnconnectedState) {
        m_reconnectTimer->start();
        emit disconnected();
    }
    reDebug() << "New state for: " << printSelf() << " --> " <<
        QMetaEnum::fromType<QModbusDevice::State>().valueToKey(state);
}

void Master::onReadReady()
{
    executeNext();
    auto rawReply = qobject_cast<QModbusReply *>(sender());
    if (!rawReply) return;
    QScopedPointer<QModbusReply, QScopedPointerDeleteLater> reply{rawReply};
    if (reply->error() != QModbusDevice::NoError) {
        reError() << printSelf() << ": Error Reading: " << reply->errorString();
        return;
    }
    auto wordsVec = reply->result().values();
    auto words = wordsVec.data();
    auto table = reply->result().registerType();
    JsonDict resultJson;
    for (int i = 0; i < wordsVec.size();) {
        if (!m_reverseRegisters[table].contains(reply->result().startAddress() + i)) {
            ++i;
            continue;
        }
        auto index = reply->result().startAddress() + i;
        const auto &registersName = m_reverseRegisters[table][index];
        const auto &regData = config().registers[registersName];
        auto sizeWords = QMetaType::sizeOf(regData.type)/2;
        auto result = parseModbusType(words + i, regData, sizeWords, config().endianess);
        i += sizeWords;
        resultJson.insert(registersName.split(':'), result);
    }
    emit sendMsg(prepareMsg(resultJson));
    emit requestDone();
}

void Master::onWriteReady()
{
    executeNext();
    auto rawReply = qobject_cast<QModbusReply *>(sender());
    if (!rawReply) return;
    QScopedPointer<QModbusReply, QScopedPointerDeleteLater> reply{rawReply};
    if (reply->error() == QModbusDevice::NoError) {
        reInfo() << printSelf() << "; Written: " << reply->result().values();
    } else {
        reError() << printSelf() << ": Error Writing: " << reply->errorString();
    }
    emit requestDone();
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

void Master::enqeueRead(const QModbusDataUnit &unit)
{
    m_readQueue.enqueue(unit);
    executeNext();
}

void Master::enqeueWrite(const QModbusDataUnit &unit)
{
    m_writeQueue.enqueue(unit);
    executeNext();
}

void Master::executeRead(const QModbusDataUnit &unit)
{
    if (auto reply = m_device->sendReadRequest(unit, m_settings.slave_id)) {
        if (!reply->isFinished()) {
            QObject::connect(reply, &QModbusReply::finished, this, &Master::onReadReady);
        } else {
            delete reply;
        }
    } else {
        reError() << printSelf() << "Read Error: " << m_device->errorString();
    }
}

void Master::executeWrite(const QModbusDataUnit &unit)
{
    auto reply = m_device->sendWriteRequest(unit, m_settings.slave_id);
    if (reply) {
        QObject::connect(reply, &QModbusReply::finished, this, &Master::onWriteReady);
    }
}
