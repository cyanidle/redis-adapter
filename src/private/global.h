#ifndef RADAPTER_BROKER_GLOBAL_H
#define RADAPTER_BROKER_GLOBAL_H

#include <QObject>
#include <QMetaMethod>

#define RADAPTER_VERSION "0.3"

#ifndef RADAPTER_SHARED_SRC
#define RADAPTER_SHARED_SRC
#endif

typedef QMap<QString /*encoded string*/, QString /*decoded value*/> ParsingMap;
template<class T>
using QStringMap = QMap<QString, T>;

#endif
