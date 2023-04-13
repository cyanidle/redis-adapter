#ifndef BROKER_H
#define BROKER_H

#include "private/global.h"
#include <QMap>
#include <QHash>
namespace Settings {
struct Broker;
}
namespace Radapter {
class Worker;
class WorkerProxy;
class Interceptor;
class WorkerMsg;
struct BrokerPrivate;
class RADAPTER_API Broker : public QObject
{
    Q_OBJECT
public:
    static Broker* instance();
    template <class Target> Target* getWorker(const QString &workerName);
    template <typename Target> QSet<Target*> getAll();
    QSet<Worker*> getAll(const QMetaObject *mobj);
    bool exists(const QString &workerName) const;
    void registerWorker(Worker *worker);
    Worker* getWorker(const QString &workerName);
    void registerInterceptor(const QString &name, Interceptor *interceptor);
    Interceptor *getInterceptor(const QString &name) const;
    void connectTwo(const QString &producer, const QString &consumer, const QStringList &interceptorNames = {});
    void connectTwo(Worker *producer, Worker *consumer, const QList<Interceptor*> interceptors = {});
    bool areConnected(Worker *producer, Worker *consumer);
    void disconnect(Worker *producer, Worker *consumer);
    void applySettings(const Settings::Broker &newSettings);
    void runAll();
    ~Broker();
signals:
    void broadcastToAll(const Radapter::WorkerMsg &msg);
protected:
    void connectProxyToWorker(WorkerProxy *producer, Worker *consumer);
private slots:
    void onMsgFromWorker(const Radapter::WorkerMsg &msg);
    void proxyDestroyed(QObject *proxy);
private:
    explicit Broker();
    BrokerPrivate *d;
};

template<typename Target>
QSet<Target*> Broker::getAll()
{
    QSet<Target*> result;
    auto found = getAll(&Target::staticMetaObject);
    for (auto worker: found) {
        result.append(qobject_cast<Target*>(worker));
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
