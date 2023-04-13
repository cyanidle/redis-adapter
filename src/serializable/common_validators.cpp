#include "common_validators.h"
#include "validator_fetch.h"

void Validator::registerAllCommon()
{
    makeFetchable<Minutes>("minutes");
    makeFetchable<Hours24>("hours24", "hours");
    makeFetchable<Hours12>("hours12");
    makeFetchable<DayOfWeek>("weekday", "day_of_week");
}
