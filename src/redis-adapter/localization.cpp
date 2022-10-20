#include "localization.h"
#include "radapterlogging.h"

Localization::Localization(const Settings::LocalizationInfo &localizationInfo, QObject *parent)
    : QObject(parent),
      m_info(localizationInfo)
{
    if (!m_info.time_zone.isValid()) {
        reDebug() << "Localization: incorrect timezone. Using the system default one...";
        m_info.time_zone = QTimeZone::systemTimeZone();
    }
}

Localization *Localization::instance()
{
    return prvInstance();
}

Localization *Localization::prvInstance(const Settings::LocalizationInfo &info, QObject *parent)
{
    static Localization localization(info, parent);
    return &localization;
}

void Localization::init(const Settings::LocalizationInfo &localizationInfo, QObject *parent)
{
    prvInstance(localizationInfo, parent);
}

QTimeZone Localization::timeZone() const
{
    return m_info.time_zone;
}
