#include "modbusscheduler.h"

using namespace Modbus;

#define MIN_SLEEP_DELAY_MS 50

ModbusScheduler::ModbusScheduler(QObject *parent)
    : QObject(parent),
      m_isStopped(false),
      m_activeQuery(nullptr),
      m_queue{},
      m_pollTable{}
{
    m_sleepTimer = new QTimer(this);
    m_sleepTimer->setSingleShot(true);
    m_sleepTimer->callOnTimeout(this, &ModbusScheduler::runQueue);
}

void ModbusScheduler::start()
{
    if (isRunning()) {
        return;
    }
    m_isStopped = false;
    runQueue();
}

void ModbusScheduler::stop()
{
    m_isStopped = true;
    m_queue.clear();
}

void ModbusScheduler::append(ModbusQuery *query)
{
    if (!m_pollTable.contains(query)) {
        auto pollItem = PollItem{ query, query->pollRate(), QDateTime{} };
        m_pollTable.insert(query, pollItem);
    }
}

void ModbusScheduler::push(ModbusQuery *query)
{
    if (!m_queue.contains(query)) {
        m_queue.prepend(query);
    }
}

void ModbusScheduler::remove(ModbusQuery *query)
{
    if (m_queue.contains(query)) {
        m_queue.removeAll(query);
    }
    if (m_pollTable.contains(query)) {
        m_pollTable.remove(query);
    }
}

void ModbusScheduler::runQueue()
{
    enqueue();
    if (m_queue.isEmpty()) {
        sleep();
        return;
    }

    m_activeQuery = m_queue.takeFirst();
    connect(m_activeQuery, &ModbusQuery::finished, this, &ModbusScheduler::onQueryFinished);
    m_activeQuery->execOnce();
}

void ModbusScheduler::enqueue()
{
    if (m_isStopped) {
        return;
    }
    auto pending = pendingQueries();
    if (pending.isEmpty()) {
        return;
    }
    for (auto &query : pending) {
        if (!m_queue.contains(query)) {
            m_queue.append(query);
        }
    }
}

void ModbusScheduler::onQueryFinished()
{
    updatePollItem(m_activeQuery);
    resetActiveQuery();
    runQueue();
}

void ModbusScheduler::updatePollItem(ModbusQuery *query)
{
    if (!m_pollTable.contains(query)) {
        return;
    }
    PollItem &pollItem = m_pollTable[query];
    pollItem.lastPollTime = now();
}

void ModbusScheduler::resetActiveQuery()
{
    disconnect(m_activeQuery, nullptr, this, nullptr);
    m_activeQuery = nullptr;
}

void ModbusScheduler::sleep()
{
    m_sleepTimer->start(nextPollDelay());
}

qint32 ModbusScheduler::nextPollDelay()
{
    qint32 minDelay = -1;
    for (auto &pollItem : m_pollTable) {
        auto nextPollTime = pollItem.lastPollTime.addMSecs(pollItem.rate);
        auto delay = now().msecsTo(nextPollTime);
        if ((delay > 0) && ((minDelay < 0) || (delay < minDelay))) {
            minDelay = static_cast<qint32>(delay);
        }
    }
    if (minDelay < 0) {
        minDelay = MIN_SLEEP_DELAY_MS;
    }
    return minDelay;
}

QList<ModbusQuery*> ModbusScheduler::pendingQueries() const
{
    auto pendingQueries = QList<ModbusQuery*>{};
    for (auto &pollItem : m_pollTable) {
        auto pollDelay = pollItem.lastPollTime.isValid() ? pollItem.lastPollTime.msecsTo(now()) : -1;
        if ((pollDelay < 0) || (pollDelay >= pollItem.rate)) {
            pendingQueries.append(pollItem.query);
        }
    }
    return pendingQueries;
}

bool ModbusScheduler::isRunning() const
{
    return !m_queue.isEmpty();
}

QDateTime ModbusScheduler::now() const
{
    return QDateTime::currentDateTime();
}
