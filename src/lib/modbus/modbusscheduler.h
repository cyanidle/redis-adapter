#ifndef MODBUSSCHEDULER_H
#define MODBUSSCHEDULER_H

#include <QObject>
#include <QDateTime>
#include "modbusquery.h"

namespace Modbus {
    class RADAPTER_SHARED_SRC ModbusScheduler;
}
class Modbus::ModbusScheduler : public QObject
{
    Q_OBJECT
public:
    explicit ModbusScheduler(QObject *parent = nullptr);

    bool isRunning() const;

public slots:
    void start();
    void stop();
    void append(Modbus::ModbusQuery *query);
    void push(Modbus::ModbusQuery *query);
    void remove(Modbus::ModbusQuery *query);

signals:

private slots:
    void runQueue();
    void enqueue();
    void onQueryFinished();
    void updatePollItem(Modbus::ModbusQuery *query);
    void resetActiveQuery();
    void sleep();
    qint32 nextPollDelay();

private:
    struct PollItem {
        ModbusQuery* query;
        quint16 rate;
        QDateTime lastPollTime;
    };
    typedef QMap<ModbusQuery*, PollItem> PollTable;

    QList<ModbusQuery*> pendingQueries() const;
    QDateTime now() const;

    bool m_isStopped;
    QTimer* m_sleepTimer;
    ModbusQuery* m_activeQuery;
    QList<ModbusQuery*> m_queue;
    PollTable m_pollTable;
};

#endif // MODBUSSCHEDULER_H
