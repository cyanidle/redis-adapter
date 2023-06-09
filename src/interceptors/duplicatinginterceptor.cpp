#include "duplicatinginterceptor.h"
#include "broker/workers/private/workermsg.h"
#include "settings/duplicatinginterseptorsettings.h"

namespace Radapter {

DuplicatingInterceptor::DuplicatingInterceptor(const Settings::DuplicatingInterceptor &settings) :
    m_settings(m_settings.create(settings))
{
}

Interceptor *DuplicatingInterceptor::newCopy() const
{
    return new DuplicatingInterceptor(*m_settings);
}

void DuplicatingInterceptor::onMsgFromWorker(WorkerMsg &msg)
{
    for (auto iter = m_settings->_by_field.cbegin(); iter != m_settings->_by_field.cend(); ++iter) {
        auto key = iter.key().split(':');
        msg[iter.value()] = msg[key];
    }
    emit msgFromWorker(msg);
}

} // namespace Radapter
