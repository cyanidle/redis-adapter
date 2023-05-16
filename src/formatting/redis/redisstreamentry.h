#ifndef REDIS_STREAMENTRY_H
#define REDIS_STREAMENTRY_H

#include "private/global.h"


namespace Redis {

struct StreamEntry {
    quint64 timestamp;
    quint64 id;
    QVariantMap values;

    StreamEntry(const QVariantList &source);
    QString streamId() const;
};


} // namespace Redis

#endif // REDIS_STREAMENTRY_H
