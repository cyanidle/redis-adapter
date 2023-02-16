#ifndef SYNC_CHANNEL_H
#define SYNC_CHANNEL_H

#include <QObject>
#include <QHash>
#include <QTimer>
namespace Radapter {
namespace Sync {
class Channel : public QObject {
    Q_OBJECT
public:
    enum Priority : quint16 {
        NormalPriority = 100,
        LowPriority = 50,
        HighPriority = 200,
    };
    Channel(QThread *thread);
    //! \warning not threadsafe
    void registerUser(QObject *user, Priority priority = NormalPriority);
    //! \note Everything else is (with connections)
    QObject *whoIsBusy() const;
    template<typename User>
    QMetaObject::Connection callOnTrigger(User *user, void (User::*slot)(QObject *)) {
        return connect(this, &Channel::trigger, user, slot);
    }
    template<typename User>
    QMetaObject::Connection signalJobDone(User *user, void (User::*signal)()) {
        return connect(user, signal, this, &Channel::onJobDone);
    }
    template<typename User>
    QMetaObject::Connection askTriggerOn(User *user, void (User::*signal)()) {
        return connect(user, signal, this, &Channel::askTrigger);
    }
signals:
    void trigger(QObject *user, QPrivateSignal);
private slots:
    void onJobDone();
    void askTrigger();
private:
    bool isBusy() const;
    void checkWaiting(QObject *ignore = nullptr);
    void activate(QObject *who);

    struct FilterIgnored;
    struct UserState {
        QObject *user;
        Priority priority;
        bool waitingForTrigger;
        bool isWaiting() const {return waitingForTrigger;}
        Priority whatPrio() const {return priority;}
    };
    QHash<QObject*, UserState> m_userStates{};
    QObject *m_busy{nullptr};
    QTimer *m_debug{nullptr};
};
}
}
#endif