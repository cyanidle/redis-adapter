#include "workerproxy.h"
#include "workermsg.h"
#include "radapterlogging.h"
#include "../worker.h"

using namespace Radapter;

WorkerProxy::WorkerProxy() :
    QObject()
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
    return qobject_cast<Worker *>(parent());
}

void WorkerProxy::onMsgFromWorker(const Radapter::WorkerMsg &msg)
{
    // If proxies are already connected, do not forward with broker
    if (msg.isDirect()) {
        auto consumers = worker()->consumers();
        auto receivers = msg.receivers();
        auto direct = consumers.intersect(receivers);
        auto forwarded = receivers.subtract(consumers);
        if (!direct.isEmpty()) {
            auto directMsg = msg;
            directMsg.setFlag(WorkerMsg::MsgDirect, false);
            directMsg.receivers().swap(direct);
            emit msgToBroker(directMsg);
        }
        if (!forwarded.isEmpty()) {
            auto forwardedMsg = msg;
            forwardedMsg.receivers().swap(forwarded);
            emit msgToBroker(forwardedMsg);
        }
    } else {
        emit msgToBroker(msg);
    }
}

const QString WorkerProxy::proxyName() const
{
    return worker()->workerName();
}

void WorkerProxy::onMsgFromBroker(const Radapter::WorkerMsg &msg)
{
    if (!msg.isValid()) {
        return;
    }
    if (msg.isBroadcast()) {
        emit msgToWorker(msg);
        return;
    }
    if (msg.isDirect()) {
        if (msg.receivers().contains(worker())) {
            emit msgToWorker(msg);
        }
        return;
    }
    emit msgToWorker(msg);
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
