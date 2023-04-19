#include "common_validators.h"
#include "validator_fetch.h"

void Validator::registerAllCommon()
{
    Validator::Fetched::initialize();
    makeFetchable<Minutes>("minutes", "minute");
    makeFetchable<Hours24>("hours24", "hours");
    makeFetchable<Hours12>("hours12");
    makeFetchable<DayOfWeek>("weekday", "day_of_week");
    makeFetchable<LogLevel>("log_level", "loglevel");
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
