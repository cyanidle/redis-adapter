#ifndef RADAPTER_BROKER_GLOBAL_H
#define RADAPTER_BROKER_GLOBAL_H

#include <QObject>
#include <QMap>
#include <QHash>
#include <QString>
#include <QVariant>
#include <type_traits>
#include <stdexcept>

#define RADAPTER_VERSION "0.85"

#ifndef RADAPTER_API
#define RADAPTER_API
#endif

template<class T>
using QStringMap = QMap<QString, T>;
using NestedKey = QString; // in format: key:key:[index]

#endif
