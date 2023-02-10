#include "resourcemonitor.h"
#include <QFile>
#include "radapterlogging.h"

#ifndef _MSC_VER
#define GET_USAGE_TIMEOUT_MS    10000
#define CPU_IDLE_INDEX          3
#define RUSAGE_STATUS_OK        0
#define SYSCONF_STATUS_ERROR    -1
#include <sys/resource.h>
#include <sys/time.h>

ResourceMonitor::ResourceMonitor(QObject *parent)
    : QObject(parent),
      m_lastOverallIdleTime{},
      m_lastOverallTotalTime{},
      m_lastUserTime{},
      m_lastSystemTime{}
{
    m_readStatsTimer = new QTimer(this);
    m_readStatsTimer->setSingleShot(false);
    m_readStatsTimer->setInterval(GET_USAGE_TIMEOUT_MS);
    m_readStatsTimer->callOnTimeout(this, &ResourceMonitor::readStats);
}

void ResourceMonitor::run()
{
    m_readStatsTimer->start();
}

void ResourceMonitor::readStats()
{
    readOverallCpuLoad();
    readProcessUsage();
}

void ResourceMonitor::readProcessUsage()
{
    rusage processStats{};
    auto status = getrusage(RUSAGE_SELF, &processStats);
    if (status != RUSAGE_STATUS_OK) {
        resmonDebug() << "rusage error: failed to get process stats";
        return;
    }
    auto processUserTime = static_cast<double>(processStats.ru_utime.tv_sec)
            + (static_cast<double>(processStats.ru_utime.tv_usec) / 1000000.0);
    auto processUserDiff = processUserTime - m_lastUserTime;
    m_lastUserTime = processUserTime;
    auto processSystemTime = static_cast<double>(processStats.ru_stime.tv_sec)
            + (static_cast<double>(processStats.ru_stime.tv_usec) / 1000000.0);
    auto processSystemDiff = processSystemTime - m_lastSystemTime;
    m_lastSystemTime = processSystemTime;
    auto processTimeDiff = processUserDiff + processSystemDiff;
    auto processLoad = (processTimeDiff / (GET_USAGE_TIMEOUT_MS / 1000)) * 100.0;
    auto processMemory = static_cast<double>(processStats.ru_maxrss) / 1024;
    auto printMessage = QString("Process - CPU: %1% MEM: %2MiB").arg(processLoad, 0, 'f', 2)
            .arg(processMemory, 0, 'f', 2);
    resmonDebug() << printMessage.toStdString().c_str();
}

void ResourceMonitor::readOverallCpuLoad()
{
    QFile procFile("/proc/stat");
    procFile.open(QFile::ReadOnly);
    // cat /proc/stat
    // cpu  32521049 4781029 14520722 2446419794 9558089 0 393177 0 0 0
    auto cpuTimes = procFile.readLine().simplified().split(' ').mid(1);
    auto idleTime = cpuTimes.at(CPU_IDLE_INDEX).toLongLong();
    auto idleDiff = static_cast<double>(idleTime - m_lastOverallIdleTime);
    m_lastOverallIdleTime = idleTime;

    qint64 totalTime = 0;
    for (auto &time : cpuTimes) {
        totalTime += time.toLongLong();
    }
    auto totalDiff = static_cast<double>(totalTime - m_lastOverallTotalTime);
    m_lastOverallTotalTime = totalTime;

    auto load = (1.0 - idleDiff / totalDiff) * 100.0;
    auto printMessage = QString("Overall - CPU: %1%").arg(load, 0, 'f', 2);
    resmonDebug() << printMessage.toStdString().c_str();
}
#endif
