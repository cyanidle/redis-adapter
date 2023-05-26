#include "modbusmaster.h"
#include "broker/broker.h"
#include "qthread.h"
#include "sync/channel.h"
#include <QModbusRtuSerialClient>
#include "settings/modbussettings.h"
#include "commands/rediscommands.h"
#include "templates/algorithms.hpp"
#include "consumers/rediscacheconsumer.h"
#include "producers/rediscacheproducer.h"
#include "jsondict/jsondict.h"
#include "modbusparsing.h"
#include <QModbusReply>
#include "sync/syncjson.h"
#include <QModbusClient>
#include <QStringBuilder>
#include <QModbusTcpClient>
#include <QModbusReply>
#include <QQueue>
#include <QTimer>
#include <QObject>

#define MAX_RECONNECTS 5

using namespace Modbus;
using namespace Radapter;

struct RegisterMetaInfo {
    QString name;
    quint8 rewriteAttempts{0};
};

struct Master::Private{
    Settings::ModbusMaster settings;
    QHash<QModbusDataUnit::RegisterType, QHash<int, QString>> reverseRegisters;
    QMap<QString, RegisterMetaInfo> regsMetaInfo;
    QQueue<QModbusDataUnit> readQueue;
    QQueue<QModbusDataUnit> writeQueue;
    QList<QModbusDataUnit> queries;
    Sync::Json state;
    QTimer *reconnectTimer;
    QTimer *readTimer;
    QModbusClient *device{nullptr};
    std::atomic<bool> connected{false};
    Redis::CacheProducer *stateWriter{nullptr};
    Redis::CacheConsumer *stateReader{nullptr};
    int reconnectAttempts{0};
};

Master::Master(const Settings::ModbusMaster &settings, QThread *thread) :
    Radapter::Worker(settings.worker, thread),
    d(new Private)
{
    d->settings = settings;
    d->reconnectTimer = new QTimer(this);
    d->readTimer = new QTimer(this);
    d->reconnectTimer->setInterval(settings.reconnect_timeout_ms);
    d->reconnectTimer->callOnTimeout(this, &Master::connectDevice);
    d->reconnectTimer->setSingleShot(true);
    d->readTimer->setInterval(settings.poll_rate);
    d->readTimer->callOnTimeout(this, &Master::doRead);
    connect(this, &Master::connected, [this]{
        d->connected=true;
        doRead();
    });
    connect(this, &Master::disconnected, [this]{
        d->connected=false;
    });
    for (auto [name, reg]: keyVal(d->settings.m_registers)) {
        if (d->reverseRegisters[reg.table].contains(reg.index)) {
            throw std::invalid_argument("Register index collission on: " +
                                        name.toStdString() +
                                        "; With --> " +
                                        d->reverseRegisters[reg.table][reg.index].toStdString() +
                                        " (Table: "+ printTable(reg.table).toStdString() +
                                        "; Register: " + QString::number(reg.index.value).toStdString() + ")");
        }
        d->reverseRegisters[reg.table][reg.index] = name;
    }
    for (auto [key, reg]: keyVal(d->settings.m_registers)) {
        d->regsMetaInfo[key] = RegisterMetaInfo{key};
        if (reg.readable) {
            d->queries.append(QModbusDataUnit{reg.table, reg.index, 1});
        }
    }
    d->queries = Modbus::mergeDataUnits(d->queries);
}

void Master::initClient()
{
    if (d->settings.m_device.tcp.wasUpdated()) {
        d->device = new QModbusTcpClient(this);
        d->device->setConnectionParameter(QModbusDevice::NetworkAddressParameter, d->settings.m_device.tcp->host.value);
        d->device->setConnectionParameter(QModbusDevice::NetworkPortParameter, d->settings.m_device.tcp->port.value);
    } else {
        d->device = new QModbusRtuSerialClient(this);
        d->device->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, d->settings.m_device.rtu->baud);
        d->device->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, d->settings.m_device.rtu->data_bits);
        d->device->setConnectionParameter(QModbusDevice::SerialParityParameter, d->settings.m_device.rtu->parity);
        d->device->setConnectionParameter(QModbusDevice::SerialPortNameParameter, d->settings.m_device.rtu->port_name.value);
        d->device->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, d->settings.m_device.rtu->stop_bits);
    }
    d->device->setTimeout(d->settings.response_time);
    d->device->setNumberOfRetries(d->settings.retries);
    connect(d->device, &QModbusDevice::stateChanged, this, &Master::onStateChanged);
    connect(d->device, &QModbusDevice::errorOccurred, this, &Master::onErrorOccurred);
}

void Master::onRun()
{
    if (d->settings.state_reader.wasUpdated()) {
        d->stateReader = broker()->getWorker<Redis::CacheConsumer>(d->settings.state_reader);
        if (!d->stateReader) {
            throw std::runtime_error(printSelf().toStdString() + ": Could not fetch RedisCacheConsumer: " + d->settings.state_reader->toStdString());
        }
        d->stateReader->waitConnected(this);
    }
    if (d->settings.state_writer.wasUpdated()) {
        d->stateWriter = broker()->getWorker<Redis::CacheProducer>(d->settings.state_writer);
        if (!d->stateWriter) {
            throw std::runtime_error(printSelf().toStdString() + ": Could not fetch RedisCacheProducer: " + d->settings.state_writer->toStdString());
        }
        d->stateWriter->waitConnected(this);
    }
    initClient();
    attachToChannel();
    connectDevice();
    fetchState();
    if (config().poll_rate) {
        d->readTimer->start();
    }
    Worker::onRun();
}

Master::~Master()
{
    d->device->disconnectDevice();
    delete d;
}

bool Master::isConnected() const
{
    return d->connected;
}

void Master::attachToChannel()
{
    auto channel = d->settings.m_device.channel.data();
    channel->registerUser(this);
    channel->callOnTrigger(this, &Master::executeNext);
    channel->signalJobDone(this, &Master::queryDone);
    channel->signalJobDone(this, &Master::allQueriesDone);
    channel->askTriggerOn(this, &Master::queryDone);
    channel->askTriggerOn(this, &Master::askTrigger);
}

const Settings::ModbusMaster &Master::config() const
{
    return d->settings;
}

void Master::saveState(const JsonDict &state)
{
    if (d->stateWriter) {
        auto command = prepareCommand(new Redis::Cache::WriteObject(workerName(), state));
        command.receivers() = {d->stateWriter};
        command.ignoreReply();
        emit sendMsg(command);
    }
}

void Master::fetchState()
{
    if (d->stateReader) {
        auto command = prepareCommand(new Redis::Cache::ReadObject(workerName()));
        command.setCallback(this, [this](const Redis::Cache::ReadObject::WantedReply *reply){
            d->state.updateTarget(reply->json());
            d->state.updateCurrent(reply->json());
        });
        command.receivers() = {d->stateReader};
        emit sendMsg(command);
    }
}

void Master::onMsg(const Radapter::WorkerMsg &msg)
{
    write(msg);
}

void Master::write(const JsonDict &data)
{
    if (!d->connected) {
        reDebug() << printSelf() << "Write while not connected!";
    }
    QList<QModbusDataUnit> results;
    for (auto&[key, value] : data) {
        if (!value.isValid()) continue;
        auto fullKeyJoined = key.join(":");
        if (!d->settings.m_registers.contains(fullKeyJoined)) {
            workerWarn(this) << "Extra property passed: " << fullKeyJoined;
            continue;
        }
        const auto &regInfo = d->settings.m_registers[fullKeyJoined];
        if (!regInfo.writable) {
            workerInfo(this) << "Attempt to write to protected register:" << fullKeyJoined;
            continue;
        }
        auto valCopy = value;
        if (regInfo.validator.wasUpdated() && !regInfo.validator->validate(valCopy)) {
            workerWarn(this, .nospace())
                << "Property: '" << fullKeyJoined
                << "' was invalidated by Validator[" << regInfo.validator.value.name()
                << "] --> value: " << value;
            continue;
        }
        if (valCopy.canConvert(QMetaType(regInfo.type))) {
            if (d->state.target().isEmpty()) {
                if (d->state.current()[key] == valCopy) continue; //do not write if already same value
            } else {
                if (d->state.target()[key] == valCopy) continue; //do not write if target is same
            }
            d->state.updateTarget(key, valCopy);
            results.append(parseValueToDataUnit(valCopy, regInfo));
        } else {
            workerError(this) << "Incompatible value under:" << fullKeyJoined << " --> " << valCopy << "; Wanted: " << regInfo.type.value;
        }
    }
    for (const auto &state : mergeDataUnits(results)) {
        enqeueWrite(state);
    }
}

void Master::connectDevice()
{
    if (!d->device->connectDevice()) {
        reDebug() << printSelf() << ": Error Connecting: " << d->device->errorString();
        d->reconnectTimer->start();
    } else {
        if (!d->writeQueue.isEmpty() || !d->readQueue.isEmpty()) {
            emit askTrigger();
        }
    }
}

void Master::executeNext()
{
    if (!d->connected) {
        emit allQueriesDone();
        return;
    }
    if (!d->writeQueue.isEmpty()) {
        executeWrite(d->writeQueue.dequeue());
    } else if (!d->readQueue.isEmpty()) {
        executeRead(d->readQueue.dequeue());
    } else {
        emit allQueriesDone();
    }
}

void Master::onErrorOccurred(QModbusDevice::Error error)
{
    workerError(this) << QMetaEnum::fromType<QModbusDevice::Error>().valueToKey(error) << "; Source: " << sender();
}

void Master::reconnect() {
    if (!d->connected) return;
    auto time = d->settings.reconnect_timeout_ms * (1 + d->reconnectAttempts);
    workerWarn(this) << ": Reconnecting in"<< time/1000.0<<"...";
    if (d->reconnectAttempts < MAX_RECONNECTS) {
        d->reconnectAttempts++;
    }
    d->device->disconnectDevice();
    d->reconnectTimer->setInterval(time);
    d->reconnectTimer->start();
}

void Master::onStateChanged(QModbusDevice::State state)
{
    if (state == QModbusDevice::UnconnectedState) {
        emit disconnected();
        d->reconnectTimer->start();
    } else if (state == QModbusDevice::ConnectedState) {
        emit connected();
    }
    workerInfo(this) << "New State --> " << QMetaEnum::fromType<QModbusDevice::State>().valueToKey(state);
}

void Master::onReadReady()
{
    auto rawReply = qobject_cast<QModbusReply *>(sender());
    if (!rawReply) {
        return;
    }
    QScopedPointer<QModbusReply, QScopedPointerDeleteLater> reply{rawReply};
    if (reply->error() != QModbusDevice::NoError) {
        workerError(this, .noquote()) << ": Error Reading:\n" << reply->errorString();
        reconnect();
        return;
    }
    d->reconnectAttempts = 0;
    auto words = reply->result().values();
    auto table = reply->result().registerType();
    JsonDict resultJson;
    for (int i = 0; i < words.size();) {
        auto startAddr = reply->result().startAddress();
        if (!d->reverseRegisters[table].contains(startAddr + i)) {
            ++i;
            continue;
        }
        auto index = reply->result().startAddress() + i;
        const auto &registersName = d->reverseRegisters[table][index];
        const auto &regData = config().m_registers[registersName];
        auto sizeWords = QMetaType(regData.type).sizeOf()/2;
        if (i + sizeWords > words.size()) {
            break;
        }
        auto result = parseModbusType(words.data() + i, regData, sizeWords);
        i += sizeWords;
        resultJson.insert(registersName.split(':'), result);
    }
    updateCurrent(resultJson);
}

void Master::onWriteReady()
{
    auto rawReply = qobject_cast<QModbusReply*>(sender());
    if (!rawReply) {
        return;
    }
    QScopedPointer<QModbusReply, QScopedPointerDeleteLater> reply{rawReply};
    auto unit = reply->result();
    auto err = reply->error();
    if (err == QModbusDevice::NoError) {
        d->reconnectAttempts = 0;
        workerInfo(this, .nospace().noquote()) << ": Written:\n" << printUnit(unit);
    } else {
        workerError(this, .nospace().noquote()) << ": Error Writing. Reason: " << reply->errorString();
        reconnect();
    }
}

void Master::doRead()
{
    if (!d->connected) {
        return;
    }
    for (auto &query: d->queries) {
        enqeueRead(query);
    }
}

void Master::updateCurrent(const JsonDict &json)
{
    auto delta = d->state.updateCurrent(json);
    if (!delta.isEmpty()) {
        emit send(delta);
    }
    auto toRewrite = d->state.missingToTarget();
    for (auto &[key, val] : toRewrite) {
        auto joined = key.join(':');
        auto &reg = d->settings.m_registers[joined];
        if (!reg.readable) {
            d->state.updateTarget(key, val);
            continue;
        }
        auto &rewriteAttempts = d->regsMetaInfo[joined].rewriteAttempts;
        if (rewriteAttempts < d->settings.retries) {
            rewriteAttempts++;
            workerWarn(this) << "Rewriting property:" << joined
                             << "; Value:" << val.toString()
                             << "; Attemtps:" << rewriteAttempts;
            toRewrite[key] = val;
        } else {
            d->state.updateTarget(key, d->state.current()[key]);
            rewriteAttempts = 0;
        }
    }
    saveState(d->state.current());
    if (!toRewrite.isEmpty()) {
        write(toRewrite);
    } else {
        d->state.target().clear();
    }
}

void Master::enqeueRead(const QModbusDataUnit &unit)
{
    for (auto &query: d->readQueue) {
        if (query.registerType() == unit.registerType() &&
            query.startAddress() == unit.startAddress() &&
            query.valueCount() == unit.valueCount())
        {
            return;
        }
    }
    d->readQueue.enqueue(unit);
    emit askTrigger();
}

void Master::enqeueWrite(const QModbusDataUnit &state)
{
    d->writeQueue.enqueue(state);
    emit askTrigger();
}

void Master::executeRead(const QModbusDataUnit &unit)
{
    if (auto reply = d->device->sendReadRequest(unit, d->settings.slave_id)) {
        connect(reply, &QModbusReply::destroyed, this, [this](QObject *obj){
            Q_UNUSED(obj)
            emit queryDone();
        });
        if (!reply->isFinished()) {
            QObject::connect(reply, &QModbusReply::finished, this, &Master::onReadReady);
        } else {
            delete reply;
        }
    } else {
        emit queryDone();
        if (d->connected) {
            workerError(this) << "Read Error: " << d->device->errorString() << "; Reconnecting...";
            reconnect();
        }
    }
}

void Master::executeWrite(const QModbusDataUnit &state)
{
    workerInfo(this, .noquote()) << "Writing...:\n" << printUnit(state);
    if (auto reply = d->device->sendWriteRequest(state, d->settings.slave_id)) {
        connect(reply, &QModbusReply::destroyed, this, [this](QObject *obj){
            Q_UNUSED(obj)
            emit queryDone();
        });
        if (!reply->isFinished()) {
            QObject::connect(reply, &QModbusReply::finished, this, &Master::onWriteReady);
        } else {
            delete reply;
        }
    } else {
        emit queryDone();
        if (d->connected) {
            workerInfo(this) << "Write Error: " << d->device->errorString() << "; Reconnecting...";
            reconnect();
        }
    }
}



