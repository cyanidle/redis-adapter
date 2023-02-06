#ifndef BROKER_LOGGING_H
#define BROKER_LOGGING_H

#include "global.h"
#include <QLoggingCategory>
#include <QDebug>

RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(settingsParsingLogging);
RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(brokerLogging);
RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(bindingsLogging);

#define settingsParsingWarn() qCWarning(settingsParsingLogging)
#define brokerInfo() qCInfo(brokerLogging)
#define brokerWarn() qCWarning(brokerLogging)
#define brokerError() qCCritical(brokerLogging)
#define bindingsError() qCCritical(bindingsLogging)

#ifndef CUSTOM_MESSAGE_PATTERN
#define CUSTOM_MESSAGE_PATTERN  "[%{time yyyy-MM-dd hh:mm:ss.zzz}] [%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] "\
"%{if-category}%{category}: %{endif}%{message}"
#endif

#endif // BROKER_LOGGING_H
