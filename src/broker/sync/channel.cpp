#include "channel.h"
#include "templates/algorithms.hpp"
#include <QSet>
using namespace Radapter::Sync;

Channel::Channel(QThread *thread)
{
    moveToThread(thread);
}

void Channel::registerUser(QObject *user, Priority priority)
{
    if (priority != NormalPriority &&
            priority != LowPriority &&
            priority != HighPriority) {
        throw std::invalid_argument("Invalid priority for channel user!");
    }
    m_users[user]= {.user = user,
                    .priority = priority,
                    .busy = false,
                    .waitingForTrigger = false,
                    };
}

bool Channel::isBusy() const
{
    return any_of(m_users, &UserState::isBusy);
}

void Channel::onJobDone()
{
    auto checked = checkSender();
    m_users[checked].busy = false;
    checkWaiting();
}

void Channel::onJobStart()
{
    auto checked = checkSender();
    m_users[checked].busy = true;
}

void Channel::askTrigger()
{
    auto checked = checkSender();
    m_users[checked].waitingForTrigger = true;
    checkWaiting();
}

void Channel::checkWaiting()
{
    if (isBusy()) return;
    auto allWaiting = filter(&m_users, &UserState::isWaiting);
    if (allWaiting.nonePass()) return;
    auto highest = max_element(&allWaiting, &UserState::whatPrio);
    if (!highest.wasFound()) return;
    highest.result->waitingForTrigger = false;
    emit trigger(highest.result->user);
}

QObject *Channel::checkSender()
{
    if (!m_users.contains(sender())) {
        throw std::runtime_error("Channel slots must NOT be called manualy! (Or channel user not registered)");
    }
    return sender();
}

