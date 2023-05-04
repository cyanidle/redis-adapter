#ifndef COMMON_VALIDATORS_H
#define COMMON_VALIDATORS_H

#include <QVariant>
Q_DECLARE_METATYPE(QtMsgType)
namespace Validator {
template<int min, int max>
struct IntMinMax {
    static const QString &name() {
        static QString stName = "Int: Min("+QString::number(min)+") Max("+QString::number(max)+')';
        return stName;
    }
    static bool validate(QVariant &target) {
        bool ok;
        auto asInt = target.toInt(&ok);
        auto wereSame = asInt == target;
        return min <= asInt && asInt <= max && ok && wereSame;
    }
};
using Minute = IntMinMax<0, 60>;
using Hour24 = IntMinMax<0, 24>;
using Hour12 = IntMinMax<0, 12>;
using DayOfWeek = IntMinMax<1, 7>;
struct LogLevel {
    static const QString &name();
    static bool validate(QVariant &target);
};

void registerAllCommon();

}

#endif // COMMON_VALIDATORS_H
