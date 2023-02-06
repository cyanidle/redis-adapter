#include "timeutils.h"

TimeUtils::TimeUtils()
{
}

timeval TimeUtils::msecsToTimeval(qint32 milliseconds)
{
    timeval time{};
    time.tv_sec = milliseconds / 1000;
    time.tv_usec = milliseconds % 1000;
    return time;
}
