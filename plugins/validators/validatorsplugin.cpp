#include "validatorsplugin.h"
#include <QVariant>
#include <QHash>

using namespace Radapter;

QString ValidatorsPlugin::name() const
{
    return "validators";
}

bool rusWindDirection360(QVariant &src)
{
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
        if (asInt - iter.key() < 23) {
            src = iter.value();
            return true;
        }
    }
    return false;
}

QMap<Validator::Function, QStringList> ValidatorsPlugin::validators() const
{
    return {
        {rusWindDirection360, {"wind_direction_360_ru"}}
    };
}

RADAPTER_DECLARE_PLUGIN(Radapter::ValidatorsPlugin)
