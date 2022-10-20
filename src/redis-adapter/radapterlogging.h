#ifndef RADAPTERLOGGING_H
#define RADAPTERLOGGING_H

#include <QLoggingCategory>
#include <QDebug>

RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(radapter);
RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(modbus);
RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(mysql);
RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(wsockserver);
RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(wsockserverjson);
RADAPTER_SHARED_SRC Q_DECLARE_LOGGING_CATEGORY(wsockclient);

#define reDebug()   qCDebug(radapter)
#define reInfo()    qCInfo(radapter)
#define reWarn()    qCWarning(radapter)
#define reError()   qCCritical(radapter)
#define mbDebug()   qCDebug(modbus)
#define sqlDebug()  qCDebug(mysql)
#define wseDebug()  qCDebug(wsockserver)
#define wseJsonDebug() qCDebug(wsockserverjson).noquote()
#define wclDebug()  qCDebug(wsockclient)

#define RADAPTER_CUSTOM_MESSAGE_PATTERN  "[%{time yyyy-MM-dd hh:mm:ss.zzz}] [%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] "\
                                "%{if-category}%{category}: %{endif}%{message}"

//#define TEST_MODE

#endif // RADAPTERLOGGING_H
