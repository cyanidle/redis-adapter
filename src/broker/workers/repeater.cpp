#include "repeater.h"
#include "settings/repeatersettings.h"
namespace Radapter {

Repeater::Repeater(const Settings::Repeater &settings, QThread *thread) :
    Worker(settings, thread)
{

}

void Repeater::onMsg(const WorkerMsg &msg)
{
    emit sendMsg(prepareMsg(msg));
}

} // namespace Radapter
