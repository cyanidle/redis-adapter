#ifndef REDIS_CACHE_QUIERIES
#define REDIS_CACHE_QUIERIES

#include <QString>
class JsonDict;

namespace Redis {

QString subscribeTo(const QStringList &events);
QString multipleSet(const JsonDict &data);
QString updateSet(const QString &set, const QStringList &keys);

}

#endif
