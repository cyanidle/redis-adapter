#ifndef RADAPTER_BROKER_GLOBAL_H
#define RADAPTER_BROKER_GLOBAL_H

#include <QObject>

#define RADAPTER_VERSION "0.7"

#ifndef RADAPTER_API
#define RADAPTER_API
#endif

template<class T>
using QStringMap = QMap<QString, T>;

#endif
