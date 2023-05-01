#include "metainfopipe.h"
#include "broker/workers/private/workermsg.h"
#include "broker/workers/worker.h"

using namespace Radapter;

MetaInfoPipe::MetaInfoPipe()
{

}

Radapter::Interceptor *MetaInfoPipe::newCopy() const
{
    return new MetaInfoPipe();
}

void MetaInfoPipe::onMsgFromWorker(WorkerMsg &msg)
{
    JsonDict metaInfo{QVariantMap{
        {"sender", msg.sender()->printSelf()},
        {"receivers", msg.printReceivers()},
        {"flags", msg.printFlags()},
        {"command", msg.command() ? msg.command()->metaObject()->className() : "None"},
        {"reply", msg.reply() ? msg.reply()->metaObject()->className() : "None"},
        {"serviceInfo", msg.printServiceData().toVariant()},
        {"timestamp", QDateTime::currentDateTime().toString()},
        {"id", msg.id()}
    }};
    msg["__meta__"] = metaInfo.toVariant();
}

