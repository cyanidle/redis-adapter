#include "namespacewrapper.h"
#include "broker/workers/private/workermsg.h"
#include "settings/namespacewrappersettings.h"

using namespace Radapter;

struct NamespaceWrapper::Private {
    Settings::NamespaceWrapper settings;
};

NamespaceWrapper::NamespaceWrapper(const Settings::NamespaceWrapper &settings) :
    d(new Private{settings})
{

}

Radapter::Interceptor *NamespaceWrapper::newCopy() const
{
    return new NamespaceWrapper(d->settings);
}

NamespaceWrapper::~NamespaceWrapper()
{
    delete d;
}

void NamespaceWrapper::onMsgFromWorker(WorkerMsg &msg)
{
    auto was = msg.json();
    msg.json().clear();
    msg.json()[d->settings.wrap_into] = was.toVariant();
    if (!msg.isEmpty()) {
        emit msgFromWorker(msg);
    }
}

