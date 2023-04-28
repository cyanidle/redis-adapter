#include "repeaterworker.h"
#include "settings/repeatersettings.h"
namespace Radapter {

struct Repeater::Private {
    Settings::Repeater settings;
};

Repeater::Repeater(const Settings::Repeater &settings, QThread *thread) :
    Worker(settings, thread),
    d(new Private{settings})
{

}

Repeater::~Repeater()
{
    delete d;
}

void Repeater::onMsg(const WorkerMsg &msg)
{
    auto copy = prepareMsg(msg);
    copy.setId(msg.id());
    if (d->settings.prevent_loopback) {
        copy.receivers().remove(msg.sender());
    }
    emit sendMsg(copy);
}

} // namespace Radapter
