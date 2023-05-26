#ifndef SYNC_CHANNEL_H
#define SYNC_CHANNEL_H

#include "private/global.h"

namespace Radapter {
namespace Sync {
class Channel : public QObject
{
    Q_OBJECT
    struct Private;
public:
    enum Priority : quint16 {
        NormalPriority = 100,
        LowPriority = 50,
        HighPriority = 200,
    };
    Channel(QThread *thread, quint32 frameGap);
    ~Channel();
    //! \warning not threadsafe
    void registerUser(QObject *user, Priority priority = NormalPriority);
    //! \note Everything else is (with connections)
    QObject *whoIsBusy() const;
    template<typename User, typename...Args>
    QMetaObject::Connection callOnTrigger(User *user, void (User::*slot)(Args...args)) {
        return connect(this,
                       &Channel::trigger,
                       user,
                       [user, slot](QObject *target){
                            if (user == target) {
                                (user->*slot)();
                            }
                       }, Qt::QueuedConnection);
    }
    template<typename User, typename...Args>
    QMetaObject::Connection signalJobDone(User *user, void (User::*signal)(Args...args)) {
        return connect(user, signal, this, &Channel::onJobDone, Qt::QueuedConnection);
    }
    template<typename User, typename...Args>
    QMetaObject::Connection askTriggerOn(User *user, void (User::*signal)(Args...args)) {
        return connect(user, signal, this, &Channel::askTrigger, Qt::QueuedConnection);
    }
signals:
    void trigger(QObject *user, QPrivateSignal);
private slots:
    void onJobDone();
    void askTrigger();
private:
    bool isBusy() const;
    void chooseNext();
    void activate(QObject *who);

    Private *d;
};
}
}
#endif
