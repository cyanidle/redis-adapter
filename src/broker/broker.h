#ifndef BROKER_H
#define BROKER_H

#include <QMutex>
#include <QMutexLocker>
#include "worker/workerproxy.h"
#include "worker/worker.h"
#include "private/global.h"

namespace Radapter {

class RADAPTER_SHARED_SRC Broker : public QObject
{
    Q_OBJECT
public:
    static Broker* instance();
    bool exists(const QString &workerName) const;
    Worker* getWorker(const QString &workerName);
    bool wasStarted();
    void registerProxy(WorkerProxy* proxy);
    void connectProducersAndConsumers();
    void runAll();
    void publishEvent(const Radapter::BrokerEvent &event);
    void connectTwoProxies(const QString &producer,
                           const QString &consumer);
    bool isDebugMode(const QString &workerName);
    //! \warning Not Thread-Safe
    void addDebugMode(const QMap<QString, bool> &table);
signals:
    void fireEvent(const Radapter::BrokerEvent &event);
    void broadcastToAll(const Radapter::WorkerMsg &msg);
protected:
    void connectTwoProxies(WorkerProxy *producer, WorkerProxy *consumer);
private slots:
    void onEvent(const Radapter::BrokerEvent &event);
    void onMsgFromWorker(const Radapter::WorkerMsg &msg);
    void proxyDestroyed(QObject *proxy);
private:
    explicit Broker();
    QMap<QString, WorkerProxy*> m_proxies;
    QList<QPair<WorkerProxy*, WorkerProxy*>> m_connected;
    bool m_wasMassConnectCalled;
    QHash<QString, bool> m_debugTable;
    static QRecursiveMutex m_mutex;
};


}

inline Radapter::Broker *Radapter::Broker::instance() {
    static Broker* broker {new Broker()};
    return broker;
}

#endif //BROKER_H
