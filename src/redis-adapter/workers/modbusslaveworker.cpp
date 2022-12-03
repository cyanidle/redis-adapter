#include "modbusslaveworker.h"
#include <QModbusRtuSerialSlave>
#include <QModbusTcpServer>

using namespace Modbus;
using namespace Radapter;
using namespace Formatters;

SlaveWorker::SlaveWorker(const Settings::ModbusSlaveWorker &settings, QThread *thread) :
    WorkerBase(settings.worker.asWorkerSettings(thread)),
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
        modbusDevice->setConnectionParameter(QModbusDevice::NetworkPortParameter, settings.device.tcp.port);
        modbusDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter, settings.device.tcp.ip);
    } else {
        modbusDevice = new QModbusRtuSerialSlave(this);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter, settings.device.rtu.port_name);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter, settings.device.rtu.parity);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, settings.device.rtu.baud);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, settings.device.rtu.data_bits);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, settings.device.rtu.stop_bits);
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
    for (int i = 0; i < size; ++i) {
        if (!m_reverseRegisters.contains(table)) {
            reWarn() << "No mapping for Table: " << table
                     << ": Adress: " << address << "; Size : " << size;
            return;
        }
        if (!m_reverseRegisters[table].contains(address)) {
            continue;
        }
        const auto &regString = m_reverseRegisters[table][address];
        auto regInfo = deviceRegisters().value(regString);
        auto result = parseType(table, address, regInfo, &i);
        if (regInfo.table == QModbusDataUnit::Coils) {
            result = result.toUInt() > 0 ? 1 : 0;
        }
        if (result.isValid()) {
            msg[regString.split(":")] = result;
        } else {
            reWarn() << "Modbus Slave error: Current adress: " << address;
        }
    }
    emit sendMsg(msg);
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
    for (auto& iter : msg) {
        auto fullKeyJoined = iter.getFullKey().join(":");
        if (m_settings.registers.contains(fullKeyJoined)) {
            auto regInfo = m_settings.registers.value(fullKeyJoined);
            auto value = iter.value();
            if (value.canConvert(regInfo.type)) {
                setValues(value, regInfo);
            } else {
                reWarn() << "Incorrect value type for slave: " << workerName() << "; Received: " << value;
            }
        }
    }
}

void SlaveWorker::setValues(const QVariant &src, const Settings::RegisterInfo &regInfo)
{
    if (!src.canConvert(regInfo.type)) {
        reError() << "Worker: " << workerName()
                  << "Error writing data to modbus slave: "
                  << src << "; Index: " << regInfo.index;
        return;
    }
    const auto sizeWords = QMetaType::sizeOf(regInfo.type)/2;
    auto copy = src;
    copy.convert(regInfo.type);
    auto wordArr = reinterpret_cast<quint16 *>(copy.data());
    applyEndianess(wordArr, sizeWords, regInfo.endianess);
    QVector<quint16> toWrite;
    for (int i = 0; i < sizeWords; ++i) {
        reDebug() << workerName() << ": Writing: " << *(wordArr + i) << " --> " << regInfo.index + i;
        toWrite.append(*(wordArr + i));
    }
    modbusDevice->setData(QModbusDataUnit{regInfo.table, regInfo.index, toWrite});
}

// Аяяйяйяйяйяйййя убили SlaveWorker`а убили,
// аяаяаяаяаяаяаая ни за что ни про что
void SlaveWorker::run()
{
    thread()->start();
}
