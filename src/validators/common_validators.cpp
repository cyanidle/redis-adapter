#include "common_validators.h"
#include "qdatetime.h"
#include "validator_fetch.h"

bool hectaPascalsToAtmospheric(QVariant &src, const QVariantList &args, QVariant &state)
{
    Q_UNUSED(args)
    Q_UNUSED(state)
    bool ok;
    auto asDouble = src.toDouble(&ok);
    if (!ok) return false;
    auto actual = asDouble * 0.75;
    src.setValue(actual);
    return true;
}

bool multiplyBy(QVariant &src, const QVariantList &args, QVariant &state)
{
    Q_UNUSED(args)
    Q_UNUSED(state)
    bool ok;
    auto asInt = src.toInt(&ok);
    if (!ok) return false;
    auto actual = asInt * args[0].toDouble(&ok);
    if (!ok) throw std::runtime_error("Args to int_divide_by must be: [<double>]");
    src.setValue(actual);
    return true;
}

bool divideBy(QVariant &src, const QVariantList &args, QVariant &state)
{
    Q_UNUSED(args)
    Q_UNUSED(state)
    bool ok;
    auto asInt = src.toInt(&ok);
    if (!ok) return false;
    auto actual = asInt / args[0].toDouble(&ok);
    if (!ok) throw std::runtime_error("Args to int_divide_by must be: [<double>]");
    src.setValue(actual);
    return true;
}

bool rusWindDirection360(QVariant &src, const QVariantList &args, QVariant &state)
{
    Q_UNUSED(args)
    Q_UNUSED(state)
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

bool setUnixTimeStamp(QVariant &src, const QVariantList &args, QVariant &state)
{
    Q_UNUSED(args)
    Q_UNUSED(state)
    src.setValue(QDateTime::currentSecsSinceEpoch());
    return true;
}

bool roundLast(QVariant &src, const QVariantList &args, QVariant &state)
{
    bool ok;
    auto sampleLength = args[0].toInt(&ok);
    if (!ok) throw std::runtime_error("round_last args must be: [<int>]");
    auto asList = state.value<QList<double>>();
    auto asDouble = src.toDouble(&ok);
    if (!ok) return false;
    asList.append(asDouble);
    while (asList.size() > sampleLength) {
        asList.pop_front();
    }
    auto sum = std::accumulate(asList.cbegin(), asList.cend(), 0);
    src.setValue(sum/asList.size());
    state.setValue(std::move(asList));
    return true;
}

bool invalidate(QVariant &src, const QVariantList &args, QVariant &state)
{
    Q_UNUSED(args)
    Q_UNUSED(state)
    src.clear();
    return true;
}

bool min_max(QVariant &src, const QVariantList &args, QVariant &state)
{
    Q_UNUSED(args)
    Q_UNUSED(state)
    bool ok;
    auto min = args[0].toDouble(&ok);
    if (!ok) throw std::runtime_error("Signature for 'min_max' is min_max(<double>, <double>)!");
    auto max = args[1].toDouble(&ok);
    if (!ok) throw std::runtime_error("Signature for 'min_max' is min_max(<double>, <double>)!");
    auto asNumb = src.toDouble(&ok);
    return ok && min <= asNumb && asNumb <= max;
}

void Validator::registerAllCommon()
{
    Validator::Fetched::initialize();
    makeFetchable<Minutes>("minutes", "minute");
    makeFetchable<Hours24>("hours24", "hours");
    makeFetchable<Hours12>("hours12");
    makeFetchable<DayOfWeek>("weekday", "day_of_week");
    makeFetchable<LogLevel>("log_level", "loglevel");
    Validator::Private::add(min_max, "min_max");
    Validator::Private::add(invalidate, "invalidate", "discard");
    Validator::Private::add(roundLast, "round_last");
    Validator::Private::add(setUnixTimeStamp, "set_unix_timestamp");
    Validator::Private::add(rusWindDirection360, "wind_direction_360_ru");
    Validator::Private::add(divideBy, "int_divide_by", "divide_by");
    Validator::Private::add(multiplyBy, "multiply_by");
    Validator::Private::add(hectaPascalsToAtmospheric, "hecta_pascals_to_mm_atmospheric");
}

bool Validator::LogLevel::validate(QVariant &target, const QVariantList &args, QVariant &state)
{
    Q_UNUSED(args)
    Q_UNUSED(state)
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
