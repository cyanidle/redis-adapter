#include "workerproxy.h"
#include "workermsg.h"
#include "radapterlogging.h"
#include "../worker.h"

using namespace Radapter;

WorkerProxy::WorkerProxy(Worker *parent) :
    QObject(parent)
{
}

const QSet<Worker *> &WorkerProxy::consumers() const
{
    return worker()->consumers();
}
const QSet<Worker*> &WorkerProxy::producers() const
{
    return worker()->producers();
}

QStringList WorkerProxy::consumersNames() const
{
    return worker()->consumersNames();
}

QStringList WorkerProxy::producersNames() const
{
    return worker()->producersNames();
}

QThread *WorkerProxy::workerThread() const
{
    return worker()->workerThread();
}

Worker *WorkerProxy::worker() const
{
    return reinterpret_cast<Worker *>(parent());
}

void WorkerProxy::onMsgFromWorker(Radapter::WorkerMsg &msg)
{
    if (msg.isBroadcast()) {
        emit msgToBroker(msg); //! on broadcast just send to broker
        return;
    }
    auto receivers = msg.receivers();
    auto forwarded = receivers.subtract(worker()->consumers());
    if (!forwarded.isEmpty() ) {
        auto copy = msg;
        copy.receivers() = forwarded;
        emit msgToBroker(copy); //! everyone except direct consumers
    }
    emit msgToConsumers(msg);
}

const QString WorkerProxy::proxyName() const
{
    return worker()->workerName();
}

void WorkerProxy::addProducers(const QStringList &producers)
{
    worker()->addProducers(producers);
}

void WorkerProxy::addConsumers(const QSet<Worker *> &consumers)
{
    worker()->addConsumers(consumers);
}

void WorkerProxy::addProducer(Worker *producer)
{
    worker()->addProducer(producer);
}

void WorkerProxy::addConsumer(Worker *consumer)
{
    worker()->addConsumer(consumer);
}

void WorkerProxy::addConsumers(const QStringList &consumers)
{
    worker()->addConsumers(consumers);
}

void WorkerProxy::addProducers(const QSet<Worker *> &producers)
{
    worker()->addProducers(producers);
}
