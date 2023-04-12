#ifndef COMMON_VALIDATORS_H
#define COMMON_VALIDATORS_H

#include <QVariant>

namespace Validator {

template<int min, int max>
struct IntMinMax {
    static bool validate(QVariant &target) {
        bool ok;
        auto asInt = target.toInt(&ok);
        auto wereSame = asInt == target;
        return min <= asInt && asInt <= max && ok && wereSame;
    }
};

using Minutes = IntMinMax<0, 60>;
using Hours24 = IntMinMax<0, 24>;
using Hours12 = IntMinMax<0, 12>;
using DayOfWeek = IntMinMax<1, 7>;

void registerAllCommon();

}

#endif // COMMON_VALIDATORS_H
