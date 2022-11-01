#ifndef MODBUSCLIENT_H
#define MODBUSCLIENT_H

#include <QObject>
#include <QModbusClient>
#include "modbusquery.h"
#include "modbusscheduler.h"
#include "modbusdeviceinfo.h"
#include "redis-adapter/settings/modbussettings.h"

namespace Modbus {
    typedef QMap<quint16 /*regAddress*/, quint16 /*regValue*/> ModbusRegistersTable;
    typedef QMap<QModbusDataUnit::RegisterType, ModbusRegistersTable> ModbusRegistersTableMap;
    typedef QVector<quint16> RegistersTableBuffer;
    typedef QMap<QModbusDataUnit::RegisterType, RegistersTableBuffer> DeviceBuffer;
    typedef QMap<quint8 /* slaveAddress */, DeviceBuffer> DeviceBuffersMap;

    struct RADAPTER_SHARED_SRC  ModbusQueryInfo {
        QModbusDataUnit dataUnit;
        quint16 pollRate;
    };

    typedef QMultiMap<quint8 /*slaveAddress*/, ModbusQueryInfo> ModbusQueryMap;

    struct RADAPTER_SHARED_SRC ModbusConnectionState {
        quint8 errorsCounter;
        bool isConnected;
    };
    typedef QMap<quint8 /*slaveAddress*/, ModbusConnectionState> ModbusConnectionStatesMap;

    class RADAPTER_SHARED_SRC ModbusClient;
}

class Modbus::ModbusClient : public QObject
{
    Q_OBJECT
public:
    explicit ModbusClient(const Settings::ModbusConnectionSource &settings,
                          const Modbus::ModbusQueryMap &queryMap,
                          const QStringList &deviceNames,
                          QObject *parent = nullptr);
    virtual ~ModbusClient() override;

    void run();

    QString channelId() const;
    QString currentSerialPort() const;
    QString description() const;
    Settings::ModbusConnectionType type() const;
    QString sourceName() const;

    Modbus::ModbusDeviceInfoList devicesInfo() const;
    QVector<quint8> devicesIdList() const;

    bool isConnected() const;
signals:
    void dataChanged(const quint8 deviceId, const Modbus::ModbusRegistersTableMap &registersMap);
    void connectionChanged(bool state);
    void writeRequested(const quint8 slaveAddress, const QModbusDataUnit &request);
    void writeResultReady(const QStringList &deviceNames,
                        const quint8 slaveAddress,
                        const QModbusDataUnit::RegisterType tableType,
                        const quint16 startAddress,
                        bool hasSucceeded);
    void readQueriesFinished();
    void firstReadDone();
    void allSlavesDisconnected();
    void slaveStateChanged(const quint8 slaveAddress, bool connected);

protected:
    void setConnected(bool state);

public slots: 
    void writeReg(const quint8 slaveAddress,
                const QModbusDataUnit::RegisterType tableType,
                const quint16 address,
                const quint16 value);
    void writeRegs(const quint8 slaveAddress,
                const QModbusDataUnit::RegisterType tableType,
                const quint16 startAddress,
                const QVector<quint16> &values);
    void changeData(const quint8 slaveAddress,
                    const QModbusDataUnit::RegisterType tableType,
                    const Modbus::ModbusRegistersTable &registersTable);

    void restartDevice();

#ifdef TEST_MODE
    void sendData(); // test
    void initRequest(); // test
#endif

private slots:
    void deviceConnect();
    void deviceDisconnect();
    void startPoll();
    void stopSlavePoll(const quint8 slaveAddress);
    void deviceStateChanged(const QModbusDevice::State state);
    void readReady(const quint8 slaveAddress, const QModbusDataUnit reply);
    void queryFailed(const quint8 slaveAddress, const QString error);
    void sendWriteRequest(const quint8 slaveAddress, const QModbusDataUnit &request);
    void sendWriteResult();
    void clearWriteQuery();
    void outputDataUnit(const quint8 slaveAddress, const QModbusDataUnit unit);
    void emitFirstReadDone();
    void countErrors(const quint8 slaveAddress);
    void resetConnectionState(const quint8 slaveAddress);
    void resetAllConnections();
    void updateDisconnectedState();
    void onSlaveStateChanged(const quint8 slaveAddress, bool connected);

private:
    void init();
    QModbusClient* createDevice();
    void setConnectionParameters();
    QModbusDataUnit createRequest(const QModbusDataUnit::RegisterType tableType, const quint16 address, const quint16 value);
    ModbusReadQuery* createReadQuery(const quint8 slaveAddress, const QModbusDataUnit &request, const quint16 pollRate);
    template< typename ModbusQueryType >
    inline void destroyQuery(ModbusQueryType *query)
    {
        query->disconnect();
        auto genericQuery = qobject_cast<ModbusQuery*>(query);
        connect(genericQuery, &ModbusQuery::finished,
                genericQuery, &ModbusQuery::deleteLater);
        genericQuery->stop();
    }

    Modbus::ModbusDeviceInfo createDeviceInfo(const quint8 deviceAddress);
    bool hasDeviceInfo(const quint8 deviceAddress);

    void tryConnect();

    void updateToNewPort();
    QString searchNewPort();

    void setDeviceConnected(const quint8 slaveAddress, ModbusConnectionState *deviceState, bool connected);

    ModbusRegistersTableMap updateRegisters(DeviceBuffersMap &regMap, quint8 slaveAddress, const QModbusDataUnit &reply);

    template< typename ModbusQueryType >
    inline QList<ModbusQueryType*> getQueriesBySlaveAddress(const QList<ModbusQueryType*> &queriesList, const quint8 slaveAddress) const
    {
        auto slaveQueries = QList<ModbusQueryType*>{};
        for (auto typedQuery : queriesList) {
            auto genericQuery = qobject_cast<ModbusQuery*>(typedQuery);
            if (genericQuery->slaveAddress() == slaveAddress) {
                slaveQueries.append(typedQuery);
            }
        }
        return slaveQueries;
    }
    QList<ModbusReadQuery*> getReadQueriesBySlaveAddress(const quint8 slaveAddress) const;
    std::string registerTypeToString(const QModbusDataUnit::RegisterType registerType) const;
    QModbusClient* m_client;
    Settings::ModbusConnectionType m_connectionType;
    ModbusQueryMap m_queryDataMap;
    QList<ModbusReadQuery*> m_readQueries;
    QList<ModbusWriteQuery*> m_writeQueries;
    ModbusScheduler* m_scheduler;
    DeviceBuffersMap m_deviceBuffersMap;
    Settings::ModbusConnectionSource m_connectionSettings;
    QStringList m_deviceNames;
    ModbusDeviceInfoList m_deviceInfoList;
    QString m_remappedPort;
    quint8 m_reconnectCounter;
    ModbusConnectionStatesMap m_connectionStatesMap;
    bool m_clientConnected;
    bool m_firstReadDone;
    bool m_areAllSlavesDisconnected;
};

#endif // MODBUSCLIENT_H
