#ifndef COMMON_VALIDATORS_HPP
#define COMMON_VALIDATORS_HPP

#include <QVariant>
namespace Validate {

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
using DayOfWeekInt = IntMinMax<1, 7>;

}

#endif // COMMON_VALIDATORS_HPP
