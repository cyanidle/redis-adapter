#include "unixsignalhandler.h"
#include "radapterlogging.h"
#include <signal.h>
#include <QPointer>

using namespace Utils;

UnixSignalHandler::UnixSignalHandler(int sig, QObject *parent)
    : QObject{parent}
{
#ifdef Q_OS_UNIX
    Q_UNUSED(sig)
#else
    qFatal("Attempt to handle signal on non unix!");
#endif
}

void UnixSignalHandler::handler(int sig)
{
    Q_UNUSED(sig)
}

