#include "redissettings.h"

bool Settings::RedisStreamConsumer::validate(QVariant &target, const QVariantList &args, QVariant &state) {
    Q_UNUSED(args)
    Q_UNUSED(state)
    auto asStr = target.toString().toLower();
    if (asStr == "persistent_id") {
        target.setValue(StartPersistentId);
    } else if (asStr == "from_top") {
        target.setValue(StartFromTop);
    } else if (asStr == "from_first") {
        target.setValue(StartFromFirst);
    } else {
        return false;
    }
    return true;
}