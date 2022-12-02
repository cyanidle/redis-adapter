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
    connect(this->thread(), &QThread::started, m_reconnectTimer, QOverload<>::of(&QTimer::start));
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
    connect(modbusDevice, &QModbusServer::dataWritten,
            this, &SlaveWorker::onDataWritten);
    connect(modbusDevice, &QModbusServer::stateChanged,
            this, &SlaveWorker::onStateChanged);
    connect(modbusDevice, &QModbusServer::errorOccurred,
            this, &SlaveWorker::onErrorOccurred);
    for (auto iter = deviceRegisters().constBegin(); iter != deviceRegisters().constEnd(); ++iter) {
        if (m_reverseRegisters.contains(iter.value().table)) {
            m_reverseRegisters[iter.value().table].insert(iter.value().index, iter.key());
        } else {
            m_reverseRegisters[iter.value().table] = {{iter.value().index, iter.key()}};
        }
    }
}

void SlaveWorker::connectDevice()
{
    if (!modbusDevice->connectDevice()) {
        reWarn() << "FAIL: Attempt to connect to: " << m_settings.device.repr();
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
    QByteArray tempBuffer;
    for (int i = 0; i < size; ++i) {
        if (!m_reverseRegisters.contains(table)) {
            break;
        }
        if (!m_reverseRegisters[table].contains(address + i)) {
            break;
        }
        const auto &regString = m_reverseRegisters[table][address + i];
        auto regInfo = deviceRegisters().value(regString);
        bool ok = false;
        auto result = parseType(table, address, regInfo, &size, &ok);

        return;
    }
    reWarn() << "No mapping for Table: " << QMetaEnum::fromType<QModbusDataUnit::RegisterType>().valueToKey(table)
             << ": Adress: " << address << "; Size : " << size;
}

QVariant SlaveWorker::parseType(QModbusDataUnit::RegisterType table, int address,
                                const Settings::RegisterInfo &reg, int *currentSize, bool *ok)
{
    constexpr static int maxSize = sizeof(long long int);
    const int size = QMetaType::sizeOf(reg.type);
    if (size > maxSize) {
        *ok = false;
        return {};
    }
    *currentSize += size - 1;
    quint16 buffer[maxSize] = {};
    for (int i = 0; i < size; ++i) {
        switch (table) {
        case QModbusDataUnit::Coils:
            modbusDevice->data(QModbusDataUnit::Coils, quint16(address), buffer + i);
            break;
        case QModbusDataUnit::DiscreteInputs:
            modbusDevice->data(QModbusDataUnit::DiscreteInputs, quint16(address), buffer + i);
            break;
        case QModbusDataUnit::HoldingRegisters:
            modbusDevice->data(QModbusDataUnit::HoldingRegisters, quint16(address), buffer + i);
            break;
        case QModbusDataUnit::InputRegisters:
            modbusDevice->data(QModbusDataUnit::InputRegisters, quint16(address), buffer + i);
            break;
        default:
            reWarn() << "Invalid data written: Adress: " << address << "; Table: "
                     << QMetaEnum::fromType<QModbusDataUnit::RegisterType>().valueToKey(table);
            break;
        }
    }
    *ok = true;
    return parseBuffer(buffer, reg.type, reg.endianess);
}

QVariant SlaveWorker::parseBuffer(quint16 *wordBuffer, const int size, const Settings::PackingMode &packing)
{
    if (packing.byte_order == QDataStream::ByteOrder::BigEndian) {
        for (int i = 0; i < size; ++i) {
            std::swap(*(wordBuffer + i), *(wordBuffer + size - i - 1));
            if (packing.word_order == QDataStream::ByteOrder::BigEndian) {
                auto bytesInWord = reinterpret_cast<quint8*>(wordBuffer + i);
                std::swap(*bytesInWord, *(bytesInWord + 1));
            }
        }
    }
}

void SlaveWorker::onErrorOccurred(QModbusDevice::Error error)
{
    reWarn() << "Error: " << m_settings.device.repr() << "; Reason: " <<
        QMetaEnum::fromType<QModbusDevice::Error>().valueToKey(error);
    disconnectDevice();

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
                m_currRegisters.insert(fullKeyJoined, value);
            } else {
                reWarn() << "Incorrect value type for slave: " << workerName() << "; Received: " << value;
            }
        }
    }
}

// Аяяйяйяйяйяйййя убили SlaveWorker, убили SlaveWorker,
// аяаяаяаяаяаяаая ни за что ни про что
void SlaveWorker::run()
{
    thread()->start();
}
