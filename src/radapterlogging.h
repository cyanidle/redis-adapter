#ifndef RADAPTERLOGGING_H
#define RADAPTERLOGGING_H

#include <QLoggingCategory>
#include <QDebug>


RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(workersLogging);
RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(radapter);
#define reDebug()   qCDebug(radapter)
#define reInfo()    qCInfo(radapter)
#define reWarn()    qCWarning(radapter)
#define reError()   qCCritical(radapter)

RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(modbus);
#define mbDebug()   qCDebug(modbus)

RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(mysql);
#define sqlDebug()  qCDebug(mysql)

RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(wsockserver);
#define wsockServDebug()  qCDebug(wsockserver)

RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(wsockclient);
#define wsockClientDebug()  qCDebug(wsockclient)

RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(resmon);
#define resmonDebug() qCDebug(resmon)

RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(settingsParsingLogging);
#define settingsParsingWarn() qCWarning(settingsParsingLogging)

RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(brokerLogging);
#define brokerInfo() qCInfo(brokerLogging)
#define brokerWarn() qCWarning(brokerLogging)
#define brokerError() qCCritical(brokerLogging)

RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(bindingsLogging);
#define bindingsError() qCCritical(bindingsLogging)


#define RADAPTER_CUSTOM_MESSAGE_PATTERN  "[%{time yyyy-MM-dd hh:mm:ss.zzz}] [%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] "\
                                "%{if-category}%{category}: %{endif}%{message}"

//#define TEST_MODE

#endif // RADAPTERLOGGING_H
