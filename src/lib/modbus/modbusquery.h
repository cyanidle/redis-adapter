#ifndef MODBUSQUERY_H
#define MODBUSQUERY_H

#include <QModbusClient>
#include <QModbusReply>
#include <QTimer>

namespace Modbus {
    class RADAPTER_SHARED_SRC ModbusQuery;
    class RADAPTER_SHARED_SRC ModbusReadQuery;
    class RADAPTER_SHARED_SRC ModbusWriteQuery;
}

class Modbus::ModbusQuery : public QObject
{
    Q_OBJECT

public:
    enum RequestType {
        ModbusQueryTypeRead,
        ModbusQueryTypeWrite
    };

    enum PollMode {
        Permanent,
        Single
    };

    explicit ModbusQuery(QModbusClient *client,
                         const RequestType type,
                         const QModbusDataUnit &query,
                         const quint8 serverAddress,
                         const quint16 pollRate = 0u,
                         QObject *parent = nullptr);

    void startPoll();
    bool isFinished() const;

    quint8 slaveAddress() const;
    QModbusDataUnit request() const;
    RequestType requestType() const;
    quint16 pollRate() const;
    bool hasSucceeded() const;
    bool doneOnce() const;

signals:
    void receivedReply(const quint8 address, const QModbusDataUnit reply);
    void receivedError(const quint8 address, const QString &error);
    void finished();

public slots:
    void execOnce();
    void execOnceDelayed();
    void replyReady();
    void start();
    void pause();
    void stop();

private slots:
    void doRun();
    void captureReplyError();
    void emitError(const QString &errorString);
    void emitResponseTimeout();

private:
    void setIsFinished(bool isFinished);
    void setHasSucceeded(bool state);

    QModbusClient* m_client;
    RequestType m_type;
    QModbusDataUnit m_query;
    quint8 m_serverAddress;
    PollMode m_pollMode;
    quint16 m_pollRate;
    QTimer m_pollTimer;
    QTimer m_responseTimer;
    QTimer m_execDelayTimer;
    bool m_queryDoneOnce;
    bool m_quit;
    bool m_isFinished;
    bool m_isOk;
};

class Modbus::ModbusReadQuery : public Modbus::ModbusQuery
{
    Q_OBJECT

public:
    explicit ModbusReadQuery(QModbusClient *client,
                             const QModbusDataUnit &query,
                             const quint8 serverAddress,
                             const quint16 pollRate = 0u,
                             QObject *parent = nullptr);
};

class Modbus::ModbusWriteQuery : public Modbus::ModbusQuery
{
    Q_OBJECT

public:
    explicit ModbusWriteQuery(QModbusClient *client,
                              const QModbusDataUnit &query,
                              const quint8 serverAddress,
                              const quint16 pollRate = 0u,
                              QObject *parent = nullptr);
};

#endif // MODBUSQUERY_H
