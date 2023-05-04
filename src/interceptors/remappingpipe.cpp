#include "remappingpipe.h"
#include "broker/workers/private/workermsg.h"
#include "settings/remappingpipesettings.h"
#include <QStringBuilder>

using namespace Radapter;
struct RemappingPipe::Private {
    Settings::RemappingPipe settings;
    QStringMap<Validator::Fetched> remappersCache;
};

Radapter::Interceptor *RemappingPipe::newCopy() const
{
    return new RemappingPipe(d->settings);
}

RemappingPipe::RemappingPipe(const Settings::RemappingPipe &settings) :
    d(new Private{settings, {}})
{

}

RemappingPipe::~RemappingPipe()
{
    delete d;
}

void RemappingPipe::onMsgFromWorker(WorkerMsg &msg)
{
    for (auto [field, remap]: d->settings.remaps) {
        if (msg.contains(field)) {
            if (!d->remappersCache.contains(field)) {
                d->remappersCache[field] = Validator::Fetched("remap", {remap.from.value, remap.to.value});
            }
            auto &data = msg[field];
            if (!d->remappersCache[field].validate(data)) {
                data.clear();
            }
        }
    }
}

