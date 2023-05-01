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
    emit msgToConsumers(msg);
}

const QString WorkerProxy::proxyName() const
{
    return worker()->workerName();
}
