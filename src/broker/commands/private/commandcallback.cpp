#include "commandcallback.h"
#include "broker/workers/private/workermsg.h"
#include "broker/workers/worker.h"
#include <QThread>

const Radapter::Reply *Radapter::CommandCallback::replyFromMsg(const WorkerMsg &msg)
{
    return msg.reply();
}

const Radapter::Command *Radapter::CommandCallback::cmdFromMsg(const WorkerMsg &msg)
{
    return msg.command();
}

Radapter::CommandCallback::operator bool() const
{
    return m_ctx;
}

Radapter::CommandCallback::CommandCallback(Worker *context, std::function<void (const WorkerMsg &)> cb) :
    m_ctx(context),
    m_cb(cb)
{
}

void Radapter::CommandCallback::execute(const WorkerMsg &msg) const
{
    if (QThread::currentThread() != m_ctx->thread()) {
        reError() << "[CommandCallback] Attempt to call callback while not in requesting workers thread!";
        return;
    }
    m_cb(msg);
}

Radapter::Worker *Radapter::CommandCallback::worker() const
{
    return m_ctx;
}
