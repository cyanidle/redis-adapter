#ifndef RADAPTERLOGGING_H
#define RADAPTERLOGGING_H

#include <QLoggingCategory>
#include <QDebug>

Q_DECLARE_LOGGING_CATEGORY(radapter);
Q_DECLARE_LOGGING_CATEGORY(modbus);
Q_DECLARE_LOGGING_CATEGORY(mysql);
Q_DECLARE_LOGGING_CATEGORY(wsockserver);
Q_DECLARE_LOGGING_CATEGORY(wsockserverjson);
Q_DECLARE_LOGGING_CATEGORY(wsockclient);

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
