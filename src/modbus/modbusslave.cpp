#include "modbusslave.h"
#include <QModbusRtuSerialServer>
#include <QModbusTcpServer>
#include "broker/workers/private/workermsg.h"
#include "radapterlogging.h"
#include <QModbusServer>
#include <QTimer>
#include <QThread>
#include "modbusparsing.h"

using namespace Modbus;
using namespace Radapter;

struct Slave::Private {
    Settings::ModbusSlave settings;
    QTimer *reconnectTimer = nullptr;
    QHash<QModbusDataUnit::RegisterType, QHash<int /*index*/, QString>> reverseRegisters;
    QModbusServer *modbusDevice = nullptr;
    JsonDict state;
    std::atomic<bool> connected{false};
};

Slave::Slave(const Settings::ModbusSlave &settings, QThread *thread) :
    Worker(settings.worker, thread),
    d(new Private)
{
    d->settings = settings;
    d->reconnectTimer = new QTimer(this);
    d->reconnectTimer->setInterval(settings.reconnect_timeout_ms);
    d->reconnectTimer->setSingleShot(true);
    d->reconnectTimer->callOnTimeout(this, &Slave::connectDevice);
    connect(this->workerThread(), &QThread::started, this, &Slave::connectDevice);
    if (settings.m_device.tcp.wasUpdated()) {
        d->modbusDevice = new QModbusTcpServer(this);
        d->modbusDevice->setConnectionParameter(QModbusDevice::NetworkPortParameter, settings.m_device.tcp->port);
        d->modbusDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter, settings.m_device.tcp->host);
    } else {
        d->modbusDevice = new QModbusRtuSerialServer(this);
        d->modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter, settings.m_device.rtu->port_name);
        d->modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter, settings.m_device.rtu->parity);
        d->modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, settings.m_device.rtu->baud);
        d->modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, settings.m_device.rtu->data_bits);
        d->modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, settings.m_device.rtu->stop_bits);
    }
    d->modbusDevice->setServerAddress(settings.slave_id);
    connect(d->modbusDevice, &QModbusServer::dataWritten,
            this, &Slave::onDataWritten);
    connect(d->modbusDevice, &QModbusServer::stateChanged,
            this, &Slave::onStateChanged);
    connect(d->modbusDevice, &QModbusServer::errorOccurred,
            this, &Slave::onErrorOccurred);
    QModbusDataUnitMap regMap;
    for (auto regIter = d->settings.m_registers.constBegin(); regIter != d->settings.m_registers.constEnd(); ++regIter) {
        if (d->reverseRegisters[regIter->table].contains(regIter->index)) {
            throw std::invalid_argument("Register index collission on: " +
                                        regIter.key().toStdString() +
                                        "; With --> " +
                                        d->reverseRegisters[regIter->table][regIter->index].toStdString() +
                                        " (Table: "+ printTable(regIter->table).toStdString() +
                                        "; Register: " + QString::number(regIter->index).toStdString() + ")");
        }
        d->reverseRegisters[regIter->table][regIter->index] = regIter.key();
    }
    workerDebug(this) << "Inserting Coils: Start: 0; Count: " << settings.counts.coils;
    regMap.insert(QModbusDataUnit::Coils, {QModbusDataUnit::Coils, 0, settings.counts.coils});
    workerDebug(this) << "Inserting Holding: Start: 0; Count: " << settings.counts.holding_registers;
    regMap.insert(QModbusDataUnit::HoldingRegisters, {QModbusDataUnit::HoldingRegisters, 0, settings.counts.holding_registers});
    workerDebug(this) << "Inserting Input: Start: 0; Count: " << settings.counts.input_registers;
    regMap.insert(QModbusDataUnit::InputRegisters, {QModbusDataUnit::InputRegisters, 0, settings.counts.input_registers});
    workerDebug(this) << "Inserting DI: Start: 0; Count: " << settings.counts.di;
    regMap.insert(QModbusDataUnit::DiscreteInputs, {QModbusDataUnit::DiscreteInputs, 0, settings.counts.di});
    d->modbusDevice->setMap(regMap);
}

Slave::~Slave()
{
    disconnectDevice();
    delete d;
}

bool Slave::isConnected() const
{
    return d->connected;
}

void Slave::connectDevice()
{
    if (!d->modbusDevice->connectDevice()) {
        workerWarn(this) << ": Failed to connect: Attempt to reconnect to: " << d->settings.m_device.print();
        d->reconnectTimer->start();
    }
}

void Slave::disconnectDevice()
{
    d->modbusDevice->disconnectDevice();
}

void Slave::handleNewWords(QVector<quint16> &words, QModbusDataUnit::RegisterType table, int address, int size)
{
    JsonDict diff;
    auto wordsData = words.data();
    for (int i = 0; i < size;) {
        if (!d->reverseRegisters[table].contains(address + i)) {
            ++i;
            continue;
        }
        const auto &regString = d->reverseRegisters[table][address + i];
        const auto &regInfo = d->settings.m_registers[regString];
        auto sizeWords = QMetaType(regInfo.type).sizeOf()/2;
        if (i + sizeWords > size) {
            workerWarn(this) << "Insufficient size of value: " << sizeWords;
            continue;
        }
        auto result = parseModbusType(wordsData + i, regInfo, sizeWords);
        i += sizeWords;
        if (result.isValid()) {
            auto key = regString.split(":");
            QVariant newValue;
            if (regInfo.table == QModbusDataUnit::Coils
                || regInfo.table == QModbusDataUnit::DiscreteInputs) {
                newValue = result.toUInt() ? true : false;
            } else {
                newValue = result;
            }
            if (d->state[key] != newValue) {
                diff[key] = newValue;
                d->state[key] = newValue;
            }
        } else {
            workerWarn(this) << "Modbus Slave error: Current adress: " << address + i;
        }
    }
    if (!diff.isEmpty()) {
        emit send(diff);
    }
}

void Slave::onDataWritten(QModbusDataUnit::RegisterType table, int address, int size)
{
    QVector<quint16> words(size);
    for (int i = 0; i < size; ++i) {
        bool ok = false;
        switch (table) {
        case QModbusDataUnit::Coils:
            ok = d->modbusDevice->data(QModbusDataUnit::Coils, quint16(address + i), &(words[i]));
            break;
        case QModbusDataUnit::DiscreteInputs:
            ok = d->modbusDevice->data(QModbusDataUnit::DiscreteInputs, quint16(address + i), &(words[i]));
            break;
        case QModbusDataUnit::HoldingRegisters:
            ok = d->modbusDevice->data(QModbusDataUnit::HoldingRegisters, quint16(address + i), &(words[i]));
            break;
        case QModbusDataUnit::InputRegisters:
            ok = d->modbusDevice->data(QModbusDataUnit::InputRegisters, quint16(address + i), &(words[i]));
            break;
        default:
            workerWarn(this) << "Invalid data written: Adress: " << address + i << "; Table: " << printTable(table);
        }
        if (!ok) {
            workerWarn(this) << "Error reading data! Adress:" << address << "; Table:" << printTable(table);
            continue;
        }
    }
    handleNewWords(words, table, address, size);
}


void Slave::onErrorOccurred(QModbusDevice::Error error)
{
    workerWarn(this) << "Error: " << d->settings.m_device.print() << "; Reason: " << error;
    disconnectDevice();
    d->reconnectTimer->start();
}

void Slave::onStateChanged(QModbusDevice::State state)
{
    if (state == QModbusDevice::ConnectedState) {
        d->connected = true;
    } else {
        d->connected = false;
    }
    workerWarn(this) << "New state --> " << state;
}

void Slave::onMsg(const Radapter::WorkerMsg &msg)
{
    QList<QModbusDataUnit> results;
    for (auto& iter : msg) {
        auto fullKeyJoined = iter.key().join(":");
        if (d->settings.m_registers.contains(fullKeyJoined)) {
            d->state[fullKeyJoined] = iter.value();
            auto regInfo = d->settings.m_registers.value(fullKeyJoined);
            auto value = iter.value();
            if (Q_LIKELY(value.canConvert(QMetaType(regInfo.type)))) {
                results.append(parseValueToDataUnit(value, regInfo));
            } else {
                reWarn() << "Incorrect value type for slave: " << printSelf() << "; Received: " << value << "; Key:" << fullKeyJoined;
            }
        }
    }
    for (auto &item: mergeDataUnits(results)) {
        d->modbusDevice->setData(item);
    }
}


// Аяяйяйяйяйяйййя убили SlaveWorker`а убили,
// аяаяаяаяаяаяаая ни за что ни про что
