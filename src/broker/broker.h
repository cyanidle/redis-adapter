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
    template <class Target>
    Target* getWorker(const QString &workerName);
    bool wasStarted();
    void registerProxy(WorkerProxy* proxy);
    void connectProducersAndConsumers();
    template <typename Target>
    QList<Target*> getAll();
    void runAll();
    void publishEvent(const Radapter::BrokerEvent &event);
    void connectTwoProxies(const QString &producer,
                           const QString &consumer);
    bool isDebugEnabled(const QString &workerName, QtMsgType type);
    //! \warning Not Thread-Safe
    void applyWorkerLoggingFilters(const QMap<QString, QMap<QtMsgType, bool>> &table);
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
    QHash<QString, QHash<QtMsgType, bool>> m_debugTable;
    static QRecursiveMutex m_mutex;
};

template<typename Target>
QList<Target*> Broker::getAll()
{
    QList<Target*> result;
    for (auto &proxy : m_proxies) {
        if (proxy->worker()->is<Target>()) {
            result.append(proxy->worker()->as<Target>());
        }
    }
    return result;
}

inline Broker *Broker::instance() {
    static Broker* broker {new Broker()};
    return broker;
}

template <class Target>
Target* Broker::getWorker(const QString &workerName)
{
    return qobject_cast<Target*>(getWorker(workerName));
}

}

#endif //BROKER_H
