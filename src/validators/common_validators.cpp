#include "common_validators.h"
#include "qdatetime.h"
#include "validator_fetch.h"

bool hectaPascalsToAtmospheric(QVariant &src)
{
    bool ok;
    auto asDouble = src.toDouble(&ok);
    if (!ok) return false;
    auto actual = asDouble * 0.75;
    src.setValue(actual);
    return true;
}

bool multiplyBy(QVariant &src, double by)
{
    bool ok;
    auto asInt = src.toInt(&ok);
    if (!ok) return false;
    auto actual = asInt * by;
    src.setValue(actual);
    return true;
}

bool divideBy(QVariant &src, double by)
{
    bool ok;
    auto asInt = src.toInt(&ok);
    if (!ok) return false;
    auto actual = asInt / by;
    src.setValue(actual);
    return true;
}

bool rusWindDirection360(QVariant &src)
{
    constexpr auto halfSectorSize = 23;
    static const QMap<int, QString> dirs {
        {0, "С"},
        {45, "СВ"},
        {90, "В"},
        {135, "ЮВ"},
        {180, "Ю"},
        {225, "ЮЗ"},
        {270, "З"},
        {315, "СЗ"},
        {360, "С"}
    };
    bool ok;
    auto asInt = src.toInt(&ok);
    if (!ok) return false;
    if (!(0 <= asInt && asInt <=360)) return false;
    for (auto iter = dirs.begin(); iter != dirs.end(); ++iter) {
        if (asInt - iter.key() < halfSectorSize) {
            src = iter.value();
            return true;
        }
    }
    return false;
}

bool setUnixTimeStamp(QVariant &src)
{
    src.setValue(QDateTime::currentSecsSinceEpoch());
    return true;
}

bool roundLast(QVariant &src, QList<int> &state, int sampleLength)
{
    bool ok;
    auto asDouble = src.toDouble(&ok);
    if (!ok) return false;
    state.append(asDouble);
    while (state.size() > sampleLength) {
        state.pop_front();
    }
    auto sum = std::accumulate(state.cbegin(), state.cend(), 0);
    src.setValue(sum/state.size());
    return true;
}

bool invalidate(QVariant &src)
{
    Q_UNUSED(src);
    return false;
}

bool clear(QVariant &src)
{
    src.clear();
    return true;
}

bool min_max(QVariant &src, double min, double max)
{
    bool ok;
    auto asNumb = src.toDouble(&ok);
    return ok && min <= asNumb && asNumb <= max;
}

void Validator::registerAllCommon()
{
    registerValidator(Minute::validate, {"minutes", "minute"});
    registerValidator(Hour24::validate, {"hours24", "hours"});
    registerValidator(Hour12::validate, {"hours12"});
    registerValidator(DayOfWeek::validate, {"weekday", "day_of_week"});
    registerValidator(LogLevel::validate, {"log_level", "loglevel"});
    registerValidator(min_max, {"min_max"});
    registerValidator(invalidate, {"invalidate", "discard"});
    registerValidator(setUnixTimeStamp, {"set_unix_timestamp"});
    registerValidator(rusWindDirection360, {"wind_direction_360_ru"});
    registerValidator(divideBy, {"int_divide_by", "divide_by"});
    registerValidator(multiplyBy, {"multiply_by"});
    registerValidator(hectaPascalsToAtmospheric, {"hecta_pascals_to_mm_atmospheric"});
    registerStatefulValidator(roundLast, {"round_last"});
}

bool Validator::LogLevel::validate(QVariant &target)
{
    static QMap<QString, QtMsgType> lvls{
        {"debug", QtMsgType::QtDebugMsg},
        {"info", QtMsgType::QtInfoMsg},
        {"warning", QtMsgType::QtWarningMsg},
        {"critical", QtMsgType::QtCriticalMsg},
        {"fatal", QtMsgType::QtFatalMsg},
    };
    auto asStr = target.toString();
    if (asStr.isEmpty()) return false;
    if (!lvls.contains(asStr)) return false;
    target.setValue(lvls.value(asStr));
    return true;
}
