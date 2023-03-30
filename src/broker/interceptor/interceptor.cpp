#include "interceptor.h"
#include "broker/workers/worker.h"

using namespace Radapter;

void InterceptorBase::onMsgFromWorker(const Radapter::WorkerMsg &msg)
{
    emit msgToBroker(msg);
}

void InterceptorBase::onMsgFromBroker(const Radapter::WorkerMsg &msg)
{
    emit msgToWorker(msg);
}

InterceptorBase::InterceptorBase() :
    QObject()
{
    setObjectName("Interceptor");
}

const Worker *InterceptorBase::worker() const
{
    return qobject_cast<const Worker*>(parent());
}

QThread *InterceptorBase::thread()
{
    return parent()->thread();
}

Worker *InterceptorBase::workerNonConst() const
{
    return qobject_cast<Worker*>(parent());
}
