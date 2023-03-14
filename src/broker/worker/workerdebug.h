#ifndef WORKERDEBUG_H
#define WORKERDEBUG_H

#include <QDebug>
#include "radapterlogging.h"

#define WORKER_LOGGER(level, msgLevel, enabled) \
    for (bool qt_category_enabled = workersLogging().isEnabled(msgLevel) && enabled; qt_category_enabled; qt_category_enabled = false) \
        (QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, workersLogging().categoryName()).level()
      //^ '(' is Important to reenable auto-quoting
#define FETCH_WORKER_LOGGER(level, msgLevel, worker, ...) \
    WORKER_LOGGER(level, msgLevel, worker->printEnabled(msgLevel)).noquote()__VA_ARGS__ << \
        ("[" + worker->printSelf() + "]: ")).maybeQuote()
                                         //^ here auto-quoting is reenabled
#define workerDebug(worker, ...) FETCH_WORKER_LOGGER(debug, QtMsgType::QtDebugMsg, worker, __VA_ARGS__)
#define workerInfo(worker, ...) FETCH_WORKER_LOGGER(info, QtMsgType::QtInfoMsg, worker,  __VA_ARGS__)
#define workerWarn(worker, ...) FETCH_WORKER_LOGGER(warning, QtMsgType::QtWarningMsg, worker,  __VA_ARGS__)
#define workerError(worker, ...) FETCH_WORKER_LOGGER(critical, QtMsgType::QtCriticalMsg, worker,  __VA_ARGS__)

#endif // WORKERDEBUG_H
