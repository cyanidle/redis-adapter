#include "channel.h"
#include "radapterlogging.h"
#include "templates/algorithms.hpp"
#include <stdexcept>
#include <QSet>
using namespace Radapter::Sync;

Channel::Channel(QThread *thread) :
    m_busy(nullptr),
    m_debug(new QTimer(this)),
    m_mutex()
{
    m_debug->setInterval(1000);
    m_debug->callOnTimeout([this](){reDebug() << this << "Busy on:" << m_busy;});
    moveToThread(thread);
}

QObject *Channel::whoIsBusy() const
{
    return const_cast<QObject*>(m_busy.load());
}

void Channel::registerUser(QObject *user, Priority priority)
{
    QMutexLocker lock(&m_mutex);
    if (priority != NormalPriority &&
            priority != LowPriority &&
            priority != HighPriority) {
        throw std::invalid_argument("Invalid priority for channel user!");
    }
    m_userStates[user] = {user, priority, false,};
}

bool Channel::isBusy() const
{
    return m_busy;
}

void Channel::onJobDone()
{
    m_debug->stop();
    auto was = m_busy.load();
    m_busy = nullptr;
    checkWaiting(was);
}

void Channel::askTrigger()
{
    auto checked = sender();
    m_userStates[checked].waitingForTrigger = true;
    checkWaiting();
}

void Channel::checkWaiting(QObject *ignore)
{
    if (isBusy()) return;
    auto allWaiting = filter(m_userStates, &UserState::isWaiting);
    auto size = allWaiting.size();
    if (size > 1) {
        auto filterIgnored = as_function([ignore](const UserState &state){
                                return state.user != ignore;
                              });
        auto max = max_element(filter(allWaiting, filterIgnored), &UserState::whatPrio);
        activate(max.result->user);
    } else if (size) {
        activate(allWaiting.begin()->user);
    }
}

void Channel::activate(QObject *who)
{
    m_debug->start();
    m_userStates[who].waitingForTrigger = false;
    m_busy = who;
    emit trigger(who, {});
}
