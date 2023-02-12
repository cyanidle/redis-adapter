#ifndef REDIS_CACHE_QUIERIES
#define REDIS_CACHE_QUIERIES

#include <QString>
class JsonDict;

namespace Redis {

QString subscribeTo(const QStringList &events);
QString toMultipleSet(const JsonDict &data);
QString toUpdateSet(const QString &set, const QStringList &keys);

}

#endif
