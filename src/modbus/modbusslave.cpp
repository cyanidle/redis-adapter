#include "modbusslave.h"
#include <QModbusRtuSerialSlave>
#include <QModbusTcpServer>
#include "radapterlogging.h"
#include "modbusparsing.h"

using namespace Modbus;
using namespace Radapter;

Slave::Slave(const Settings::ModbusSlave &settings, QThread *thread) :
    Worker(settings.worker, thread),
    m_settings(settings),
    m_reconnectTimer(new QTimer(this)),
    m_reverseRegisters()
{
    m_reconnectTimer->setInterval(settings.reconnect_timeout_ms);
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->callOnTimeout(this, &Slave::connectDevice);
    connect(this->workerThread(), &QThread::started, this, &Slave::connectDevice);
    if (settings.device.tcp) {
        modbusDevice = new QModbusTcpServer(this);
        modbusDevice->setConnectionParameter(QModbusDevice::NetworkPortParameter, settings.device.tcp->port);
        modbusDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter, settings.device.tcp->host);
    } else {
        modbusDevice = new QModbusRtuSerialSlave(this);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter, settings.device.rtu->port_name);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter, settings.device.rtu->parity);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, settings.device.rtu->baud);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, settings.device.rtu->data_bits);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, settings.device.rtu->stop_bits);
    }
    modbusDevice->setServerAddress(settings.slave_id);
    connect(modbusDevice, &QModbusServer::dataWritten,
            this, &Slave::onDataWritten);
    connect(modbusDevice, &QModbusServer::stateChanged,
            this, &Slave::onStateChanged);
    connect(modbusDevice, &QModbusServer::errorOccurred,
            this, &Slave::onErrorOccurred);
    QModbusDataUnitMap regMap;
    for (auto regIter = deviceRegisters().constBegin(); regIter != deviceRegisters().constEnd(); ++regIter) {
        if (m_reverseRegisters[regIter->table].contains(regIter->index)) {
            throw std::invalid_argument("Register index collission on: " +
                                        regIter.key().toStdString() +
                                        "; With --> " +
                                        m_reverseRegisters[regIter->table][regIter->index].toStdString() +
                                        " (Table: "+ tableToString(regIter->table).toStdString() +
                                        "; Register: " + QString::number(regIter->index).toStdString() + ")");
        }
        m_reverseRegisters[regIter->table][regIter->index] = regIter.key();
    }
    workerDebug(this) << "Inserting Coils: Start: 0; Count: " << settings.counts.coils;
    regMap.insert(QModbusDataUnit::Coils, {QModbusDataUnit::Coils, 0, settings.counts.coils});
    workerDebug(this) << "Inserting Holding: Start: 0; Count: " << settings.counts.holding_registers;
    regMap.insert(QModbusDataUnit::HoldingRegisters, {QModbusDataUnit::HoldingRegisters, 0, settings.counts.holding_registers});
    workerDebug(this) << "Inserting Input: Start: 0; Count: " << settings.counts.input_registers;
    regMap.insert(QModbusDataUnit::InputRegisters, {QModbusDataUnit::InputRegisters, 0, settings.counts.input_registers});
    workerDebug(this) << "Inserting DI: Start: 0; Count: " << settings.counts.di;
    regMap.insert(QModbusDataUnit::DiscreteInputs, {QModbusDataUnit::DiscreteInputs, 0, settings.counts.di});
    modbusDevice->setMap(regMap);
}

Slave::~Slave()
{
    disconnectDevice();
}

void Slave::connectDevice()
{
    if (!modbusDevice->connectDevice()) {
        workerWarn(this) << ": Failed to connect: Attempt to reconnect to: " << m_settings.device.repr();
        m_reconnectTimer->start();
    }
}

void Slave::disconnectDevice()
{
    modbusDevice->disconnectDevice();
}

void Slave::onDataWritten(QModbusDataUnit::RegisterType table, int address, int size)
{
    auto msg = prepareMsg();
    QVector<quint16> words(size);
    for (int i = 0; i < size; ++i) {
        bool ok = false;
        switch (table) {
        case QModbusDataUnit::Coils:
            ok = modbusDevice->data(QModbusDataUnit::Coils, quint16(address + i), &(words[i]));
            break;
        case QModbusDataUnit::DiscreteInputs:
            ok = modbusDevice->data(QModbusDataUnit::DiscreteInputs, quint16(address + i), &(words[i]));
            break;
        case QModbusDataUnit::HoldingRegisters:
            ok = modbusDevice->data(QModbusDataUnit::HoldingRegisters, quint16(address + i), &(words[i]));
            break;
        case QModbusDataUnit::InputRegisters:
            ok = modbusDevice->data(QModbusDataUnit::InputRegisters, quint16(address + i), &(words[i]));
            break;
        default:
            workerWarn(this) << "Invalid data written: Adress: " << address + i << "; Table: " << tableToString(table);
        }
        if (!ok) {
            workerWarn(this) << "Error reading data! Adress:" << address << "; Table:" << tableToString(table);
            continue;
        }
    }
    auto wordsStart = words.data();
    for (int i = 0; i < size;) {
        if (!m_reverseRegisters[table].contains(address + i)) {
            ++i;
            continue;
        }
        const auto &regString = m_reverseRegisters[table][address];
        auto regInfo = deviceRegisters().value(regString);
        auto sizeWords = QMetaType::sizeOf(regInfo.type)/2;
        auto result = parseModbusType(wordsStart + i, regInfo, sizeWords, config().endianess);
        i += sizeWords;
        if (i + sizeWords < size) {
            reWarn() << "Insufficient size of value: " << sizeWords;
        }
        if (result.isValid()) {
            if (regInfo.table == QModbusDataUnit::Coils
                || regInfo.table == QModbusDataUnit::DiscreteInputs) {
                msg[regString.split(":")] = result.toUInt() ? true : false;
            } else {
                msg[regString.split(":")] = result;
            }
        } else {
            workerWarn(this) << "Modbus Slave error: Current adress: " << address + i;
        }
    }
    emit sendMsg(msg);
}


void Slave::onErrorOccurred(QModbusDevice::Error error)
{
    workerWarn(this) << "Error: " << m_settings.device.repr() << "; Reason: " << error;
    disconnectDevice();
    m_reconnectTimer->start();
}

void Slave::onStateChanged(QModbusDevice::State state)
{
    workerWarn(this) << "New state for: " << printSelf() <<" --> " << state;
}

void Slave::onMsg(const Radapter::WorkerMsg &msg)
{
    QList<QModbusDataUnit> results;
    for (auto& iter : msg) {
        auto fullKeyJoined = iter.key().join(":");
        if (m_settings.registers.contains(fullKeyJoined)) {
            auto regInfo = m_settings.registers.value(fullKeyJoined);
            auto value = iter.value();
            if (Q_LIKELY(value.canConvert(regInfo.type))) {
                results.append(parseValueToDataUnit(value, regInfo, config().endianess));
            } else {
                reWarn() << "Incorrect value type for slave: " << printSelf() << "; Received: " << value << "; Key:" << fullKeyJoined;
            }
        }
    }
    for (auto &item: mergeDataUnits(results)) {
        modbusDevice->setData(item);
    }
}


// Аяяйяйяйяйяйййя убили SlaveWorker`а убили,
// аяаяаяаяаяаяаая ни за что ни про что
