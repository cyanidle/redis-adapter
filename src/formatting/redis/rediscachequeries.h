#ifndef REDIS_CACHE_QUIERIES
#define REDIS_CACHE_QUIERIES

#include "private/global.h"

class JsonDict;

namespace Redis {

QString subscribeTo(const QStringList &events);
QString toMultipleSet(const JsonDict &data);
QString toUpdateSet(const QString &set, const QStringList &keys);

}

#endif
