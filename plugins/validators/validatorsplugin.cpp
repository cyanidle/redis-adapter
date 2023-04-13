#include "validatorsplugin.h"
#include "qdatetime.h"
#include <QVariant>
#include <QHash>

using namespace Radapter;

QString ValidatorsPlugin::name() const
{
    return "validators";
}

bool hectaPascalsToAtmospheric(QVariant &src)
{
    bool ok;
    auto asDouble = src.toDouble(&ok);
    if (!ok) return false;
    auto actual = asDouble * 0.75;
    src.setValue(actual);
    return true;
}

bool intDivideBy10(QVariant &src)
{
    bool ok;
    auto asInt = src.toInt(&ok);
    if (!ok) return false;
    auto actual = asInt / 10.;
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
    src.setValue(QDateTime::currentMSecsSinceEpoch());
    return true;
}

QMap<Validator::Function, QStringList> ValidatorsPlugin::validators() const
{
    return {
        {rusWindDirection360, {"wind_direction_360_ru"}},
        {hectaPascalsToAtmospheric, {"hecta_pascals_to_mm_atmospheric"}},
        {intDivideBy10, {"int_divide_by_10"}},
        {setUnixTimeStamp, {"set_unix_timestamp"}}
    };
}

RADAPTER_DECLARE_PLUGIN(Radapter::ValidatorsPlugin)
