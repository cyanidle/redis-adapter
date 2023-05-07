#ifndef RADAPTER_BROKER_GLOBAL_H
#define RADAPTER_BROKER_GLOBAL_H

#include <QObject>
#include <QLoggingCategory>
#include <QDebug>
#include <QMap>
#include <QHash>
#include <QSet>
#include <QString>
#include <QVariant>
#include <stdexcept>
#include <functional>
#include <type_traits>
#include <tuple>
#include <QMetaObject>

#define RADAPTER_VERSION "1.0"

#ifndef RADAPTER_API
#define RADAPTER_API
#endif

template<class T>
using QStringMap = QMap<QString, T>;
using NestedKey = QString; // in format: key:key:[index]

#endif
