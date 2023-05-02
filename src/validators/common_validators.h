#ifndef COMMON_VALIDATORS_H
#define COMMON_VALIDATORS_H

#include <QVariant>
Q_DECLARE_METATYPE(QtMsgType)
namespace Validator {
using Function = bool (*)(QVariant &target, const QVariantList &args, QVariant& state);
template<int min, int max>
struct IntMinMax {
    static bool validate(QVariant &target, const QVariantList &args, QVariant& state) {
        Q_UNUSED(args)
        Q_UNUSED(state)
        bool ok;
        auto asInt = target.toInt(&ok);
        auto wereSame = asInt == target;
        return min <= asInt && asInt <= max && ok && wereSame;
    }
};
template<int min, int max>
struct IntMinMaxSequence {
    static bool validate(QVariant &target, const QVariantList &args, QVariant& state) {
        Q_UNUSED(args)
        Q_UNUSED(state)
        auto asList = target.value<QList<qint64>>();
        auto status = !asList.isEmpty();
        for (auto &item : asList) {
            status &= (min <= item && item <= max);
        }
        return status;
    }
};
using Minutes = IntMinMax<0, 60>;
using MinutesSequence = IntMinMaxSequence<0, 60>;
using Hours24 = IntMinMax<0, 24>;
using Hours24Sequence = IntMinMaxSequence<0, 24>;
using Hours12 = IntMinMax<0, 12>;
using DayOfWeek = IntMinMax<1, 7>;
using DaysOfWeekSequence = IntMinMaxSequence<1, 7>;
struct LogLevel {
    static bool validate(QVariant &target, const QVariantList &args, QVariant &state);
};

void registerAllCommon();

}

#endif // COMMON_VALIDATORS_H
