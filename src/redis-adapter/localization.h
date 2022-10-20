#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <QObject>
#include "settings/settings.h"

class RADAPTER_SHARED_SRC Localization : public QObject
{
    Q_OBJECT
public:
    static Localization* instance();
    static void init(const Settings::LocalizationInfo &localizationInfo, QObject *parent);

    QTimeZone timeZone() const;

signals:

private:
    explicit Localization(const Settings::LocalizationInfo &localizationInfo, QObject *parent = nullptr);
    static Localization* prvInstance(const Settings::LocalizationInfo &info = Settings::LocalizationInfo{},
                                     QObject *parent = nullptr);

    Settings::LocalizationInfo m_info;
};

#endif // LOCALIZATION_H
