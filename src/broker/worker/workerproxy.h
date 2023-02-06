#ifndef WORKERPROXY_H
#define WORKERPROXY_H

#include "workermsg.h"
#include "private/global.h"

namespace Radapter {
class Worker;

class RADAPTER_SHARED_SRC WorkerProxy : public QObject
{
    Q_OBJECT
public:
    const QString proxyName() const;
    const QSet<Worker *> &consumers() const;
    const QSet<Worker *> &producers() const;
    QStringList consumersNames() const;
    QStringList producersNames() const;
    QThread *thread() const;
    Worker *worker() const;
signals:
    void msgToWorker(const Radapter::WorkerMsg &msg);
    void msgToBroker(const Radapter::WorkerMsg &msg);
public slots:
    void onMsgFromWorker(const Radapter::WorkerMsg &msg);
    void onMsgFromBroker(const Radapter::WorkerMsg &msg);
private:
    void addProducers(const QStringList &producers);
    void addConsumers(const QStringList &producers);
    void addProducers(const QSet<Worker*> &producers);
    void addConsumers(const QSet<Worker*> &consumers);
    friend class Radapter::Worker;
    friend class Radapter::Broker;
    explicit WorkerProxy();
};

}
#endif //WORKERPROXY_H
