#include "channel.h"
#include "radapterlogging.h"
#include "templates/algorithms.hpp"
#include <QSet>
using namespace Radapter::Sync;


Channel::Channel(QThread *thread) :
    m_debug(new QTimer(this))
{
    m_debug->setInterval(1000);
    m_debug->callOnTimeout([this](){reDebug() << this << "Busy on:" << m_busy;});
    moveToThread(thread);
}

QObject *Channel::whoIsBusy() const
{
    return const_cast<QObject*>(m_busy);
}

void Channel::registerUser(QObject *user, Priority priority)
{
    if (priority != NormalPriority &&
            priority != LowPriority &&
            priority != HighPriority) {
        throw std::invalid_argument("Invalid priority for channel user!");
    }
    m_userStates[user]= {.user = user,
                         .priority = priority,
                         .waitingForTrigger = false,
                         };
}

bool Channel::isBusy() const
{
    return m_busy;
}

void Channel::onJobDone()
{
    m_debug->stop();
    auto was = m_busy;
    m_busy = nullptr;
    checkWaiting(was);
}

void Channel::askTrigger()
{
    auto checked = sender();
    m_userStates[checked].waitingForTrigger = true;
    checkWaiting();
}

struct Channel::FilterIgnored {
    FilterIgnored(QObject *ignore) : m_ingored(ignore) {}
    bool operator()(const UserState &state) const {
        return state.user != m_ingored;
    }
private:
    QObject *m_ingored;
};

void Channel::checkWaiting(QObject *ignore)
{
    if (isBusy()) return;
    auto allWaiting = filter(&m_userStates, &UserState::isWaiting);
    auto size = allWaiting.size();
    if (size > 1) {
        auto highestExcept = filter(&allWaiting, FilterIgnored(ignore));
        auto max = max_element(&highestExcept, &UserState::whatPrio);
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
