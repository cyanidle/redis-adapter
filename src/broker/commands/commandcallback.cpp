#include "commandcallback.h"
#include "broker/worker/workermsg.h"

void Radapter::CallbackConcept::execute(const WorkerMsg &msg)
{
    executeMsg(msg);
    executePlain(msg.command(), msg.reply());
}
