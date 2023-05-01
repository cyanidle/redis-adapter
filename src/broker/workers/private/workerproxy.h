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
    void msgToConsumers(const Radapter::WorkerMsg &msg);
public slots:
    void onMsgFromWorker(Radapter::WorkerMsg &msg);
private:
    friend class Radapter::Worker;
    friend class Radapter::Broker;
    explicit WorkerProxy(Worker *parent);
};

}
#endif //WORKERPROXY_H
