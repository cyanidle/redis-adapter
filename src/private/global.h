#ifndef RADAPTER_BROKER_GLOBAL_H
#define RADAPTER_BROKER_GLOBAL_H

#include <QObject>

#define RADAPTER_VERSION "0.3"

#ifndef RADAPTER_API
#define RADAPTER_API
#endif

typedef QMap<QString /*encoded string*/, QString /*decoded value*/> ParsingMap;
template<class T>
using QStringMap = QMap<QString, T>;

#endif
