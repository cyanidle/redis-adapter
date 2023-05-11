#ifndef RESOURCEMONITOR_H
#define RESOURCEMONITOR_H

#include "private/global.h"

class RADAPTER_API ResourceMonitor : public QObject
{
    Q_OBJECT
#ifndef _MSC_VER
public:
    explicit ResourceMonitor(QObject *parent = nullptr);

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
#endif
};
#endif // RESOURCEMONITOR_H
