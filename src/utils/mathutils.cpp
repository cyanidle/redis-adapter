#include "mathutils.h"
#include <QtMath>

#define EQUALITY_DELTA  0.01

bool Utils::Math::isEqual(const double first, const double second)
{
    auto diff = qAbs(first - second);
    bool isEqual = diff <= EQUALITY_DELTA;
    return isEqual;
}
