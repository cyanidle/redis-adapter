#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <QObject>
#include <QMutex>
#include "settings/settings.h"

class RADAPTER_SHARED_SRC Localization
{
public:
    static Localization* instance();
    QTimeZone timeZone() const;
    QTimeZone applyInfo(const Settings::LocalizationInfo &timeZone);
private:
    Localization();
    Settings::LocalizationInfo m_info{};
    mutable QMutex m_mutex{};
};

#endif // LOCALIZATION_H
