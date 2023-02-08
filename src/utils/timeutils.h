#ifndef TIMEUTILS_H
#define TIMEUTILS_H

#include <QtGlobal>
#ifndef _MSC_VER
#include <sys/time.h> /* for struct timeval */
#else
struct timeval; /* forward declaration */
typedef long long ssize_t;
#endif
#include <QDateTime>

namespace Utils {
namespace Time {
timeval msecsToTimeval(qint32 milliseconds);
QDateTime fromString(const QString &string);
QDateTime fromTimestamp(quint64 timestamp);
QString toMsecString(const QDateTime &datetime);
}
}

#endif // TIMEUTILS_H
