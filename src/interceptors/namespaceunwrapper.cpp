#include "namespaceunwrapper.h"
#include "broker/workers/private/workermsg.h"
#include "interceptors/settings/namespaceunwrappersettings.h"

using namespace Radapter;

struct NamespaceUnwrapper::Private {
    Settings::NamespaceUnwrapper settings;
};

NamespaceUnwrapper::NamespaceUnwrapper(const Settings::NamespaceUnwrapper &settings) :
    d (new Private{settings})
{

}

Radapter::Interceptor *NamespaceUnwrapper::newCopy() const
{
    return new NamespaceUnwrapper(d->settings);
}

NamespaceUnwrapper::~NamespaceUnwrapper()
{
    delete d;
}

void NamespaceUnwrapper::onMsgFromWorker(WorkerMsg &msg)
{
    msg.json() = JsonDict(msg.json()[d->settings.unwrap_from], false);
    if (!msg.isEmpty()) {
        emit msgFromWorker(msg);
    }
}

