#ifndef RESOURCEMONITOR_H
#define RESOURCEMONITOR_H

#include <QObject>
#include <QTimer>

class RADAPTER_SHARED_SRC ResourceMonitor : public QObject
{
    Q_OBJECT
public:
    explicit ResourceMonitor(QObject *parent);

signals:

public slots:
    void run();

private slots:
    void readStats();
    void readProcessUsage();
    void readOverallCpuLoad();

private:
    QTimer* m_readStatsTimer;
    qint64 m_lastOverallIdleTime;
    qint64 m_lastOverallTotalTime;
    double m_lastUserTime;
    double m_lastSystemTime;
};

#endif // RESOURCEMONITOR_H
