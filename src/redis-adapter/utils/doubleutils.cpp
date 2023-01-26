#include "doubleutils.h"
#include <QtMath>

#define EQUALITY_DELTA  0.01

DoubleUtils::DoubleUtils()
{
}

bool DoubleUtils::isEqual(const double first, const double second)
{
    auto diff = qAbs(first - second);
    bool isEqual = diff <= EQUALITY_DELTA;
    return isEqual;
}
