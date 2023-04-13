#ifndef WORKERPROXY_H
#define WORKERPROXY_H

#include "private/global.h"

namespace Radapter {
class Worker;
class WorkerMsg;
class Broker;
class RADAPTER_API WorkerProxy : public QObject
{
    Q_OBJECT
public:
    const QString proxyName() const;
    const QSet<Worker *> &consumers() const;
    const QSet<Worker *> &producers() const;
    QStringList consumersNames() const;
    QStringList producersNames() const;
    QThread *workerThread() const;
    Worker *worker() const;
signals:
    void msgToBroker(const Radapter::WorkerMsg &msg);
    void msgToConsumers(const Radapter::WorkerMsg &msg);
public slots:
    void onMsgFromWorker(Radapter::WorkerMsg &msg);
private:
    void addProducers(const QStringList &producers);
    void addConsumers(const QStringList &producers);
    void addProducers(const QSet<Worker*> &producers);
    void addConsumers(const QSet<Worker*> &consumers);
    void addProducer(Worker* producer);
    void addConsumer(Worker* consumer);
    friend class Radapter::Worker;
    friend class Radapter::Broker;
    explicit WorkerProxy(Worker *parent);
};

}
#endif //WORKERPROXY_H
