#include "modbusslaveworker.h"
#include <QModbusRtuSerialSlave>
#include <QModbusTcpServer>

using namespace Modbus;
using namespace Radapter;
 

SlaveWorker::SlaveWorker(const Settings::ModbusSlaveWorker &settings, QThread *thread) :
    WorkerBase(settings.worker, thread),
    m_settings(settings),
    m_reconnectTimer(new QTimer(this)),
    m_reverseRegisters()
{
    m_reconnectTimer->setInterval(settings.reconnect_timeout_ms);
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->callOnTimeout(this, &SlaveWorker::connectDevice);
    connect(this->thread(), &QThread::started, this, &SlaveWorker::connectDevice);
    if (settings.device.device_type == Settings::Tcp) {
        modbusDevice = new QModbusTcpServer(this);
        modbusDevice->setConnectionParameter(QModbusDevice::NetworkPortParameter, settings.device.tcp->port);
        modbusDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter, settings.device.tcp->ip);
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
            this, &SlaveWorker::onDataWritten);
    connect(modbusDevice, &QModbusServer::stateChanged,
            this, &SlaveWorker::onStateChanged);
    connect(modbusDevice, &QModbusServer::errorOccurred,
            this, &SlaveWorker::onErrorOccurred);
    QModbusDataUnitMap regMap;
    for (auto iter = deviceRegisters().constBegin(); iter != deviceRegisters().constEnd(); ++iter) {
        if (m_reverseRegisters.contains(iter.value().table)) {
            m_reverseRegisters[iter.value().table].insert(iter.value().index, iter.key());
        } else {
            m_reverseRegisters[iter.value().table] = {{iter.value().index, iter.key()}};
        }
    }
    regMap.insert(QModbusDataUnit::Coils, {QModbusDataUnit::Coils, 0, settings.coils});
    regMap.insert(QModbusDataUnit::HoldingRegisters, {QModbusDataUnit::HoldingRegisters, 0, settings.holding_registers});
    regMap.insert(QModbusDataUnit::InputRegisters, {QModbusDataUnit::InputRegisters, 0, settings.input_registers});
    regMap.insert(QModbusDataUnit::DiscreteInputs, {QModbusDataUnit::DiscreteInputs, 0, settings.di});
    modbusDevice->setMap(regMap);
}

SlaveWorker::~SlaveWorker()
{
    disconnectDevice();
}

void SlaveWorker::connectDevice()
{
    if (!modbusDevice->connectDevice()) {
        reWarn() << workerName() << ": Failed to connect: Attempt to reconnect to: " << m_settings.device.repr();
        m_reconnectTimer->start();
    }
}

void SlaveWorker::disconnectDevice()
{
    modbusDevice->disconnectDevice();
}

void SlaveWorker::onDataWritten(QModbusDataUnit::RegisterType table, int address, int size)
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
            reWarn() << "Invalid data written: Adress: " << address + i << "; Table: " << table;
        }
        if (!ok) {
            reWarn() << "Error reading data! Adress:" << address << "; Table:" << table;
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
        auto result = parseType(wordsStart + i, regInfo, sizeWords);
        i += sizeWords;
        if (i + sizeWords < size) {
            reWarn() << "Insufficient size of value: " << sizeWords;
        }
        if (result.isValid()) {
            if (regInfo.table == QModbusDataUnit::Coils
                || regInfo.table == QModbusDataUnit::DiscreteInputs) {
                msg[regString.split(":")] = result.toUInt() ? 1 : 0;
            } else {
                msg[regString.split(":")] = result;
            }
        } else {
            reWarn() << "Modbus Slave error: Current adress: " << address;
        }
    }
    emit sendMsg(msg);
}

QVariant SlaveWorker::parseType(quint16* words, const Settings::RegisterInfo &regInfo, int sizeWords)
{
    applyEndianness(words, regInfo.endianess, sizeWords, true);
    switch(regInfo.type) {
    case QMetaType::UShort:
        return bit_cast<quint16>(words);
    case QMetaType::UInt:
        return bit_cast<quint32>(words);
    case QMetaType::Float:
        return bit_cast<float>(words);
    default:
        return{};
    }
}

void SlaveWorker::onErrorOccurred(QModbusDevice::Error error)
{
    reWarn() << "Error: " << m_settings.device.repr() << "; Reason: " <<
        QMetaEnum::fromType<QModbusDevice::Error>().valueToKey(error);
    disconnectDevice();
    m_reconnectTimer->start();
}

void SlaveWorker::onStateChanged(QModbusDevice::State state)
{
    reDebug() << "New state for: " << workerName() <<" --> " <<
        QMetaEnum::fromType<QModbusDevice::State>().valueToKey(state);
}

void SlaveWorker::onMsg(const Radapter::WorkerMsg &msg)
{
    QList<QModbusDataUnit> results;
    for (auto& iter : msg) {
        auto fullKeyJoined = iter.fullKey().join(":");
        if (m_settings.registers.contains(fullKeyJoined)) {
            auto regInfo = m_settings.registers.value(fullKeyJoined);
            auto value = iter.value();
            if (Q_LIKELY(value.canConvert(regInfo.type))) {
                results.append(parseValueToDataUnit(value, regInfo));
            } else {
                reWarn() << "Incorrect value type for slave: " << workerName() << "; Received: " << value << "; Key:" << fullKeyJoined;
            }
        }
    }
    //! \todo Merge results based on tables
    for (auto &item: results) {
        modbusDevice->setData(item);
    }
}

QModbusDataUnit SlaveWorker::parseValueToDataUnit(const QVariant &src, const Settings::RegisterInfo &regInfo)
{
    if (!src.canConvert(regInfo.type)) {
        reError() << "Worker: " << workerName()
                  << "Error writing data to modbus slave: "
                  << src << "; Index: " << regInfo.index;
        return {};
    }
    const auto sizeWords = QMetaType::sizeOf(regInfo.type)/2;
    auto copy = src;
    if (!copy.convert(regInfo.type)) {
        reWarn() << "MbSlave: Conversion error!";
        return {};
    }
    auto words = toWords(copy.data(), sizeWords);
    auto rawWords = words.data();
    applyEndianness(rawWords, regInfo.endianess, sizeWords, false);
    QVector<quint16> toWrite(sizeWords);
    for (int i = 0; i < sizeWords; ++i) {
        quint16 value = rawWords[i];
        reDebug() << workerName() << ": Writing: " << value << " --> " << regInfo.index + i;
        toWrite[i] = value;
    }
    return QModbusDataUnit{regInfo.table, regInfo.index, toWrite};
}

// Аяяйяйяйяйяйййя убили SlaveWorker`а убили,
// аяаяаяаяаяаяаая ни за что ни про что
void SlaveWorker::run()
{
    thread()->start();
}
