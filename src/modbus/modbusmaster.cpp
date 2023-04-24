#include "modbusmaster.h"
#include "broker/broker.h"
#include "broker/sync/channel.h"
#include <QModbusRtuSerialClient>
#include "commands/rediscommands.h"
#include "templates/algorithms.hpp"
#include "consumers/rediscacheconsumer.h"
#include "producers/rediscacheproducer.h"
#include <QModbusReply>
#include <QModbusClient>
#include <QModbusTcpClient>

using namespace Modbus;
using namespace Radapter;

struct Master::Private{
    Settings::ModbusMaster settings;
    QHash<QModbusDataUnit::RegisterType, QHash<int, QString>> reverseRegisters;
    QQueue<QModbusDataUnit> readQueue;
    QQueue<QModbusDataUnit> writeQueue;
    JsonDict state;
    JsonDict wantedState;
    QMap<QString, quint8> rewriteAttempts;
    QTimer *reconnectTimer;
    QTimer *readTimer;
    QModbusClient *device{nullptr};
    std::atomic<bool> connected{false};
    Redis::CacheProducer *stateWriter{nullptr};
    Redis::CacheConsumer *stateReader{nullptr};
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
            d->state.merge(reply->json());
            d->wantedState.merge(d->state);
            emit sendBasic(d->state);
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
        return;
    }
    QList<QModbusDataUnit> results;
    for (auto& iter : data) {
        if (!iter.value().isValid()) continue;
        auto fullKeyJoined = iter.key().join(":");
        if (!d->settings.registers.contains(fullKeyJoined)) continue;
        const auto &regInfo = d->settings.registers[fullKeyJoined];
        if (!regInfo.writable) {
            workerInfo(this) << "Attempt to write to protected register: " << regInfo.print();
            continue;
        }
        auto value = iter.value();
        if (regInfo.validator.isValid() && !regInfo.validator->validate(value)) {
            workerWarn(this, .nospace())
                << "Property: '" << fullKeyJoined
                << "' was invalidated by Validator[" << regInfo.validator.value.name()
                << "] --> value: " << value;
            continue;
        }
        if (value.canConvert(QMetaType(regInfo.type))) {
            d->wantedState[iter.key()] = value;
            if (d->state[iter.key()] == value) {
                continue;
            }
            results.append(parseValueToDataUnit(value, regInfo));
        } else {
            workerError(this) << "Incompatible value under:" << fullKeyJoined << " --> " << value << "; Wanted: " << regInfo.type.value;
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
        if (!d->reverseRegisters[table].contains(reply->result().startAddress() + i)) {
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
    formatAndSendJson(resultJson);
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
    if (!d->connected) {
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
    JsonDict toRewrite;
    for (auto &newJson : json) {
        auto key = newJson.key();
        auto joinedKey = key.join(':');
        auto &lastVal = d->state[key];
        auto &wantedVal = d->wantedState[key];
        auto &newVal = newJson.value();
        auto &rewriteAttempts = d->rewriteAttempts[joinedKey];
        if (newVal != lastVal) {
            lastVal = newVal;
            wantedVal = newVal;
            result[key] = newVal;
        } else if (newVal != wantedVal && rewriteAttempts < d->settings.retries) {
            rewriteAttempts++;
            workerWarn(this) << "Rewriting property:" << joinedKey
                             << "; Value:" << wantedVal
                             << "; Attemtps:" << rewriteAttempts;
            toRewrite[key] = wantedVal;
        } else {
            wantedVal = newVal;
            rewriteAttempts = 0;
        }
    }
    if (!result.isEmpty()) {
        saveState(result);
        emit sendBasic(result);
    }
    if (!toRewrite.isEmpty()) {
        write(toRewrite);
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
        workerError(this) << "Read Error: " << d->device->errorString() << "; Reconnecting...";
        emit queryDone();
        reconnect();
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
        workerInfo(this) << "Write Error: " << d->device->errorString() << "; Reconnecting...";
        emit queryDone();
        reconnect();
    }
}



