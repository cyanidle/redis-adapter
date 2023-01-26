#ifndef TIMEUTILS_H
#define TIMEUTILS_H

#include <QtGlobal>
#ifndef _MSC_VER
#include <sys/time.h> /* for struct timeval */
#else
struct timeval; /* forward declaration */
typedef long long ssize_t;
#endif

class TimeUtils
{
public:
    TimeUtils();

    static timeval msecsToTimeval(qint32 milliseconds);
};

#endif // TIMEUTILS_H
