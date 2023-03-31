#include "commandcallback.h"
#include "broker/workers/private/workermsg.h"

const Radapter::Reply *Radapter::CallbackConcept::replyFromMsg(const WorkerMsg &msg)
{
    return msg.reply();
}

const Radapter::Command *Radapter::CallbackConcept::cmdFromMsg(const WorkerMsg &msg)
{
    return msg.command();
}
