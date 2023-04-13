#include "interceptor.h"
#include "broker/workers/worker.h"

using namespace Radapter;

void Interceptor::onMsgFromWorker(WorkerMsg &msg)
{
    emit msgFromWorker(msg);
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
