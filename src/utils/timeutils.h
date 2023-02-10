#ifndef TIMEUTILS_H
#define TIMEUTILS_H

#include <QtGlobal>
#include <QDateTime>

namespace Utils {
namespace Time {
QDateTime fromString(const QString &string);
QDateTime fromTimestamp(quint64 timestamp);
QString toMsecString(const QDateTime &datetime);
}
}

#endif // TIMEUTILS_H
