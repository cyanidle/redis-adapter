#include "channel.h"
#include "radapterlogging.h"
#include "templates/algorithms.hpp"
#include <stdexcept>
#include <QSet>
#include <QMutex>
#include <QTimer>

using namespace Radapter::Sync;

static quint64 getTimeMS()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

struct Channel::Private {
    struct UserState {
        QObject *user;
        Priority priority;
        bool waitingForTrigger;

        bool isWaiting() const {
            return waitingForTrigger;
        }
        Priority whatPrio() const {
            return priority;
        }
    };
    QHash<QObject*, UserState> userStates;
    std::atomic<QObject*> busy{nullptr};
    QObject *lastBusy{nullptr};
    QTimer *debug;
    QTimer *frameTimer;
    QMutex mutex;
    quint32 frameGap;
    quint64 lastFrame;
};

Channel::Channel(QThread *thread, quint32 frameGap) :
    d(new Private)
{
    d->frameGap = frameGap;
    d->lastFrame = getTimeMS();
    d->frameTimer = new QTimer(this);
    d->frameTimer->setSingleShot(true);
    d->frameTimer->callOnTimeout(this, &Channel::chooseNext);
    d->debug = new QTimer(this);
    d->debug->setInterval(1000);
    d->debug->callOnTimeout([this]{
        reDebug() << this << "Busy on:" << d->busy;
    });
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
    connect(user, &QObject::destroyed, this, [this](QObject *obj){
        if (d->busy == obj) {
            d->busy = nullptr;
        }
        d->userStates.remove(obj);
    });
}

bool Channel::isBusy() const
{
    return d->busy;
}

void Channel::onJobDone()
{
    d->debug->stop();
    d->lastBusy = d->busy.load();
    d->busy = nullptr;
    chooseNext();
}

void Channel::askTrigger()
{
    auto checked = sender();
    d->userStates[checked].waitingForTrigger = true;
    chooseNext();
}

void Channel::chooseNext()
{
    if (isBusy()) return;
    auto passed = getTimeMS() - d->lastFrame;
    qint64 toWait = d->frameGap - passed;
    if (toWait > 0) {
        d->frameTimer->start(toWait);
        return;
    }
    auto allWaiting = filter(d->userStates, &Private::UserState::isWaiting);
    auto size = allWaiting.size();
    if (size > 1) {
        auto filterIgnored = as_function([this](const Private::UserState &state){
            return state.user != d->lastBusy;
        });
        auto max = max_element(filter(allWaiting, filterIgnored), &Private::UserState::whatPrio);
        activate(max.result->user);
    } else if (size) {
        activate(allWaiting.begin()->user);
    }
    d->lastFrame = getTimeMS();
}

void Channel::activate(QObject *who)
{
    d->debug->start();
    d->userStates[who].waitingForTrigger = false;
    d->busy = who;
    emit trigger(who, QPrivateSignal{});
}
