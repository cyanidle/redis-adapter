#include "pipestart.h"
#include "workermsg.h"
#include "workerproxy.h"

namespace Radapter {

void PipeStart::onSendMsg(const WorkerMsg &msg)
{
    if (msg.isBroadcast()) return;
    auto copy = msg;
    emit msgFromWorker(copy);
}

PipeStart::PipeStart(WorkerProxy *parent)
    : QObject{parent}
{

}

} // namespace Radapter
