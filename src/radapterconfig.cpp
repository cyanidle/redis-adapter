#include "radapterconfig.h"
#include "broker/broker.h"
#include "localization.h"

void Settings::AppConfig::postUpdate() {
    if (localization.isValid()) {
        Localization::instance()->applyInfo(localization);
    }
    if (broker.isValid()) {
        Radapter::Broker::instance()->applySettings(broker);
    }
}

void Settings::Modbus::postUpdate()
{
    for (auto [device, regs]: registers) {
        regs.init(device);
    }
}
