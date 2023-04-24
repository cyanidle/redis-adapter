#include "redissettings.h"
using namespace Settings;
bool RedisStreamConsumer::validate(QVariant &target, const QVariantList &args, QVariant &state) {
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

Q_GLOBAL_STATIC(QStringMap<RedisServer>, cacheMap)

void RedisConnector::postUpdate() {
    server = cacheMap->value(server_name);
}

void RedisServer::postUpdate() {
    cacheMap->insert(name, *this);
}
