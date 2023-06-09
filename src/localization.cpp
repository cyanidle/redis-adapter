#include "localization.h"
#include "radapterlogging.h"

Localization::Localization() :
    m_info()
{
    m_info.time_zone = QTimeZone::systemTimeZone();
}

Localization *Localization::instance()
{
    static Localization inst{};
    return &inst;
}

QTimeZone Localization::timeZone() const
{
    QMutexLocker lock(&m_mutex);
    return m_info.time_zone;
}

void Localization::applyInfo(const Settings::LocalizationInfo &timeZone)
{
    QMutexLocker lock(&m_mutex);
    if (!timeZone.time_zone->isValid()) {
        throw std::invalid_argument("Localization: incorrect timezone!");
    }
    m_info = timeZone;
}
