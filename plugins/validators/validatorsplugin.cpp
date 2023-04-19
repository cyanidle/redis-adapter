#include "validatorsplugin.h"
#include "qdatetime.h"
#include <QVariant>
#include <QHash>

using namespace Radapter;

QString ValidatorsPlugin::name() const
{
    return "validators";
}

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

bool intDivideBy(QVariant &src, const QVariantList &args, QVariant &state)
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

QMap<Validator::Function, QStringList> ValidatorsPlugin::validators() const
{
    return {
        {rusWindDirection360, {"wind_direction_360_ru"}},
        {hectaPascalsToAtmospheric, {"hecta_pascals_to_mm_atmospheric"}},
        {intDivideBy, {"int_divide_by"}},
        {setUnixTimeStamp, {"set_unix_timestamp"}},
        {roundLast, {"round_last"}},
    };
}

RADAPTER_DECLARE_PLUGIN(Radapter::ValidatorsPlugin)
