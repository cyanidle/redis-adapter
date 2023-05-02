#ifndef RADAPTERLOGGING_H
#define RADAPTERLOGGING_H

#include "private/global.h"

RADAPTER_API Q_DECLARE_LOGGING_CATEGORY(workersLogging);
RADAPTER_API Q_DECLARE_LOGGING_CATEGORY(radapter);
#define reDebug()   qCDebug(radapter)
#define reInfo()    qCInfo(radapter)
#define reWarn()    qCWarning(radapter)
#define reError()   qCCritical(radapter)

RADAPTER_API Q_DECLARE_LOGGING_CATEGORY(mysql);
#define sqlDebug()  qCDebug(mysql)

RADAPTER_API Q_DECLARE_LOGGING_CATEGORY(resmon);
#define resmonDebug() qCDebug(resmon)

RADAPTER_API Q_DECLARE_LOGGING_CATEGORY(settingsParsingLogging);
#define settingsParsingWarn() qCWarning(settingsParsingLogging)
#define settingsParsingInfo() qCInfo(settingsParsingLogging)
#define settingsParsingDebug() qCDebug(settingsParsingLogging)

RADAPTER_API Q_DECLARE_LOGGING_CATEGORY(brokerLogging);
#define brokerInfo() qCInfo(brokerLogging)
#define brokerWarn() qCWarning(brokerLogging)
#define brokerError() qCCritical(brokerLogging)


#define RADAPTER_CUSTOM_MESSAGE_PATTERN  "[%{time yyyy-MM-dd hh:mm:ss.zzz}] [%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] "\
                                "%{if-category}%{category}: %{endif}%{message}"

//#define TEST_MODE

#endif // RADAPTERLOGGING_H
