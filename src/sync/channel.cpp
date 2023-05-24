#include "channel.h"
#include "radapterlogging.h"
#include "templates/algorithms.hpp"
#include <stdexcept>
#include <QSet>
#include <QMutex>
#include <QTimer>

using namespace Radapter::Sync;

struct Channel::Private {
    struct UserState {
        QObject *user;
        Priority priority;
        bool waitingForTrigger;

        bool isWaiting() const {return waitingForTrigger;}
        Priority whatPrio() const {return priority;}
    };
    QHash<QObject*, UserState> userStates;
    std::atomic<QObject*> busy{nullptr};
    QTimer *debug;
    QMutex mutex;
};

Channel::Channel(QThread *thread) :
    d(new Private)
{
    d->debug = new QTimer(this);
    d->debug->setInterval(1000);
    d->debug->callOnTimeout([this](){reDebug() << this << "Busy on:" << d->busy;});
    moveToThread(thread);
}

Channel::~Channel()
{
    delete d;
}

QObject *Channel::whoIsBusy() const
{
    return const_cast<QObject*>(d->busy.load());
}

void Channel::registerUser(QObject *user, Priority priority)
{
    QMutexLocker lock(&d->mutex);
    if (priority != NormalPriority &&
            priority != LowPriority &&
            priority != HighPriority) {
        throw std::invalid_argument("Invalid priority for channel user!");
    }
    d->userStates[user] = {user, priority, false,};
}

bool Channel::isBusy() const
{
    return d->busy;
}

void Channel::onJobDone()
{
    d->debug->stop();
    auto was = d->busy.load();
    d->busy = nullptr;
    checkWaiting(was);
}

void Channel::askTrigger()
{
    auto checked = sender();
    d->userStates[checked].waitingForTrigger = true;
    checkWaiting();
}

void Channel::checkWaiting(QObject *ignore)
{
    if (isBusy()) return;
    auto allWaiting = filter(d->userStates, &Private::UserState::isWaiting);
    auto size = allWaiting.size();
    if (size > 1) {
        auto filterIgnored = as_function([ignore](const Private::UserState &state){
                                return state.user != ignore;
                              });
        auto max = max_element(filter(allWaiting, filterIgnored), &Private::UserState::whatPrio);
        activate(max.result->user);
    } else if (size) {
        activate(allWaiting.begin()->user);
    }
}

void Channel::activate(QObject *who)
{
    d->debug->start();
    d->userStates[who].waitingForTrigger = false;
    d->busy = who;
    emit trigger(who, QPrivateSignal{});
}
