#include "renamingpipe.h"
#include "broker/workers/private/workermsg.h"
#include "interceptors/settings/renamingpipesettings.h"

using namespace Radapter;

struct RenamingPipe::Private {
    Settings::RenamingPipe settings;
};

Interceptor *RenamingPipe::newCopy() const
{
    return new RenamingPipe(d->settings);
}

RenamingPipe::RenamingPipe(const Settings::RenamingPipe &settings) :
    d(new Private{settings})
{

}

RenamingPipe::~RenamingPipe()
{
    delete d;
}

void RenamingPipe::onMsgFromWorker(WorkerMsg &msg)
{
    for (auto [was, now]: d->settings.renames) {
        auto taken = msg.take(was);
        if (taken.isValid()) {
            msg[now] = taken;
        }
    }
}

