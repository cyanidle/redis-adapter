#ifndef BROKER_H
#define BROKER_H

#include "private/global.h"
#include <QMap>
#include <QHash>

namespace Radapter {
class BrokerSettings;
class Worker;
class WorkerProxy;
class Interceptor;
class BrokerEvent;
class WorkerMsg;
class RADAPTER_API Broker : public QObject
{
    Q_OBJECT
public:
    static Broker* instance();
    void applySettings(const BrokerSettings &newSettings);
    bool exists(const QString &workerName) const;
    Worker* getWorker(const QString &workerName);
    template <class Target>
    Target* getWorker(const QString &workerName);
    bool wasStarted();
    void registerInterceptor(const QString &name, Interceptor *interceptor);
    Interceptor *getInterceptor(const QString &name) const;
    void registerProxy(WorkerProxy* proxy);
    void connectProducersAndConsumers();
    template <typename Target>
    QList<Target*> getAll();
    void runAll();
    void publishEvent(const Radapter::BrokerEvent &event);
    void connectTwo(const QString &producer,
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
    QMap<QString, Interceptor*> m_interceptors;
    QMap<QString, WorkerProxy*> m_proxies;
    QHash<QString, QHash<QtMsgType, bool>> m_debugTable;
    QList<QPair<WorkerProxy*, WorkerProxy*>> m_connected;
    bool m_wasMassConnectCalled;
};

template<typename Target>
QList<Target*> Broker::getAll()
{
    QList<Target*> result;
    for (auto worker = m_proxies.keyBegin(); worker != m_proxies.keyEnd(); ++worker) {
        result.append(qobject_cast<Target>(getWorker(*worker)));
    }
    return result;
}

template <class Target>
Target* Broker::getWorker(const QString &workerName)
{
    return qobject_cast<Target*>(getWorker(workerName));
}


}

#endif //BROKER_H
