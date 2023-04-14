#include "radapterconfig.h"
#include "broker/broker.h"
#include "localization.h"
#include "routed_object/routesprovider.h"

void Settings::AppConfig::postUpdate() {
    if (localization.isValid()) {
        Localization::instance()->applyInfo(localization);
    }
    if (json_routes.isValid()) {
        JsonRoutesProvider::init(JsonRoute::parseMap(json_routes));
    }
    if (broker.isValid()) {
        Radapter::Broker::instance()->applySettings(broker);
    }
}
