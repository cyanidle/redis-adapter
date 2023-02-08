#ifndef SYNC_CHANNEL_H
#define SYNC_CHANNEL_H

#include <QObject>
#include <QHash>
namespace Radapter {
namespace Sync {
class Channel : public QObject {
    Q_OBJECT
public:
    enum Priority : quint16 {
        NormalPriority = 10,
        LowPriority = 0,
        HighPriority = 20,
    };
    Channel(QThread *thread);
    //! \warning not threadsafe
    void registerUser(QObject *user, Priority priority = NormalPriority);
    //! \note Everything else is (with connections)
    bool isBusy() const;
signals:
    void trigger(QObject *user);
public slots:
    void onJobDone();
    void onJobStart();
    void askTrigger();
private:
    QObject *checkSender();
    void checkWaiting();
    struct UserState {
        QObject *user;
        Priority priority;
        bool busy;
        bool waitingForTrigger;
        bool isBusy() const {return busy;}
        bool isWaiting() const {return waitingForTrigger;}
        Priority whatPrio() const {return priority;}
    };
    QHash<QObject*, UserState> m_users;
};
}
}
#endif
