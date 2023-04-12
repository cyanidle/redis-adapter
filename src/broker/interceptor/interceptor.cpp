#include "interceptor.h"
#include "broker/workers/worker.h"

using namespace Radapter;

void Interceptor::onMsgFromWorker(const Radapter::WorkerMsg &msg)
{
    emit msgToBroker(msg);
}

void Interceptor::onMsgFromBroker(const Radapter::WorkerMsg &msg)
{
    emit msgToWorker(msg);
}

Interceptor::Interceptor() :
    QObject()
{
    setObjectName("Interceptor");
}

const Worker *Interceptor::worker() const
{
    return qobject_cast<const Worker*>(parent());
}

QThread *Interceptor::thread()
{
    return parent()->thread();
}

Worker *Interceptor::workerNonConst() const
{
    return qobject_cast<Worker*>(parent());
}
