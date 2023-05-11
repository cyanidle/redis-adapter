#include "modbusmaster.h"
#include "broker/broker.h"
#include "broker/sync/channel.h"
#include <QModbusRtuSerialClient>
#include "commands/rediscommands.h"
#include "templates/algorithms.hpp"
#include "consumers/rediscacheconsumer.h"
#include "producers/rediscacheproducer.h"
#include <QModbusReply>
#include "broker/sync/syncjson.h"
#include <QModbusClient>
#include <QStringBuilder>
#include <QModbusTcpClient>

using namespace Modbus;
using namespace Radapter;

struct Master::Private{
    Settings::ModbusMaster settings;
    QHash<QModbusDataUnit::RegisterType, QHash<int, QString>> reverseRegisters;
    QQueue<QModbusDataUnit> readQueue;
    QQueue<QModbusDataUnit> writeQueue;
    Sync::Json state;
    QMap<QString, quint8> rewriteAttempts;
    QTimer *reconnectTimer;
    QTimer *readTimer;
    QModbusClient *device{nullptr};
    std::atomic<bool> connected{false};
    Redis::CacheProducer *stateWriter{nullptr};
    Redis::CacheConsumer *stateReader{nullptr};
    int currentRead{0};
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
    for (auto regIter = settings.registers.cbegin(); regIter != settings.registers.cend(); ++regIter) {
        if (d->reverseRegisters[regIter->table].contains(regIter->index)) {
            throw std::invalid_argument("Register index collission on: " +
                                        regIter.key().toStdString() +
                                        "; With --> " +
                                        d->reverseRegisters[regIter->table][regIter->index].toStdString() +
                                        " (Table: "+ printTable(regIter->table).toStdString() +
                                        "; Register: " + QString::number(regIter->index.value).toStdString() + ")");
        }
        d->reverseRegisters[regIter->table][regIter->index] = regIter.key();
    }
    connect(this, &Master::connected, [this](){d->connected=true;});
    connect(this, &Master::disconnected, [this](){d->connected=false;});
    if (!d->settings.read_only && d->settings.queries->isEmpty()) {
        throw std::runtime_error(
            QString(printSelf()%": Empty queries while not read_only. If you wanted to forbid reading set 'read_only: true'")
                .toStdString());
    }
}

void Master::initClient()
{
    if (d->settings.device.tcp.isValid()) {
        d->device = new QModbusTcpClient(this);
        d->device->setConnectionParameter(QModbusDevice::NetworkAddressParameter, d->settings.device.tcp->host.value);
        d->device->setConnectionParameter(QModbusDevice::NetworkPortParameter, d->settings.device.tcp->port.value);
    } else {
        d->device = new QModbusRtuSerialClient(this);
        d->device->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, d->settings.device.rtu->baud);
        d->device->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, d->settings.device.rtu->data_bits);
        d->device->setConnectionParameter(QModbusDevice::SerialParityParameter, d->settings.device.rtu->parity);
        d->device->setConnectionParameter(QModbusDevice::SerialPortNameParameter, d->settings.device.rtu->port_name.value);
        d->device->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, d->settings.device.rtu->stop_bits);
    }
    d->device->setTimeout(d->settings.response_time_ms);
    d->device->setNumberOfRetries(d->settings.retries);
    connect(d->device, &QModbusDevice::stateChanged, this, &Master::onStateChanged);
    connect(d->device, &QModbusDevice::errorOccurred, this, &Master::onErrorOccurred);
}

void Master::onRun()
{
    if (d->settings.state_reader.isValid()) {
        d->stateReader = broker()->getWorker<Redis::CacheConsumer>(d->settings.state_reader);
        if (!d->stateReader) {
            throw std::runtime_error(printSelf().toStdString() + ": Could not fetch RedisCacheConsumer: " + d->settings.state_reader->toStdString());
        }
        d->stateReader->waitConnected(this);
    }
    if (d->settings.state_writer.isValid()) {
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
    auto channel = d->settings.device.channel.data();
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
        command.setCallback(this, [this](const ReplyJson *reply){
            d->state.updateTarget(reply->json());
            d->state.updateCurrent(reply->json());
        });
        command.receivers() = {d->stateReader};
        emit sendMsg(command);
    }
}

void Master::onMsg(const Radapter::WorkerMsg &msg)
{
    if (d->settings.read_only) {
        workerWarn(this) << "Attempt to write while read-only! See config.";
        return;
    }
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
        if (!d->settings.registers.contains(fullKeyJoined)) continue;
        const auto &regInfo = d->settings.registers[fullKeyJoined];
        if (!regInfo.writable) {
            workerInfo(this) << "Attempt to write to protected register: " << regInfo.print();
            continue;
        }
        auto valCopy = value;
        if (regInfo.validator.isValid() && !regInfo.validator->validate(valCopy)) {
            workerWarn(this, .nospace())
                << "Property: '" << fullKeyJoined
                << "' was invalidated by Validator[" << regInfo.validator.value.name()
                << "] --> value: " << value;
            continue;
        }
        if (valCopy.canConvert(QMetaType(regInfo.type))) {
            if (!d->state.updateTarget(key, valCopy)) continue;
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
    }
}

void Master::executeNext()
{
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
    workerWarn(this) << ": Reconnecting...";
    d->device->disconnectDevice();
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
        const auto &regData = config().registers[registersName];
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
        workerInfo(this, .nospace().noquote()) << ": Written:\n" << printUnit(unit);
    } else {
        workerError(this, .nospace().noquote()) << ": Error Writing. Reason: " << reply->errorString();
        reconnect();
    }
}

void Master::doRead()
{
    if (!d->connected || !config().queries->size()) {
        return;
    }
    auto &query = config().queries[d->currentRead++];
    auto unit = QModbusDataUnit(query.type, query.reg_index, query.reg_count);
    enqeueRead(unit);
    if (d->currentRead >= config().queries->size()) {
        d->currentRead = 0;
    }
}

void Master::updateCurrent(const JsonDict &json)
{
    auto last = d->state.current();
    d->state.updateCurrent(json);
    auto toRewrite = d->state.missingToTarget();
    for (auto &[key, val] : toRewrite) {
        auto joined = key.join(':');
        auto &rewriteAttempts = d->rewriteAttempts[joined];
        if (rewriteAttempts < d->settings.retries) {
            rewriteAttempts++;
            workerWarn(this) << "Rewriting property:" << joined
                             << "; Value:" << val.toString()
                             << "; Attemtps:" << rewriteAttempts;
            toRewrite[key] = val;
        } else {
            d->state.updateCurrent(key, val);
            d->state.updateTarget(key, val);
            rewriteAttempts = 0;
        }
    }
    saveState(d->state.current());
    auto diff = d->state.current() - last;
    if (!diff.isEmpty()) {
        emit send(diff);
    }
    if (!toRewrite.isEmpty()) {
        QTimer::singleShot(d->settings.response_time_ms, this, [this, toRewrite]{
            write(toRewrite);
        });
    }
}

void Master::enqeueRead(const QModbusDataUnit &unit)
{
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
        connect(reply, &QModbusReply::destroyed, this, &Master::queryDone);
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
    if (auto reply = d->device->sendWriteRequest(state, d->settings.slave_id)) {
        workerInfo(this, .noquote()) << "Writing...:\n" << printUnit(state);
        connect(reply, &QModbusReply::destroyed, this, &Master::queryDone);
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



