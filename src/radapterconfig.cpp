#include "radapterconfig.h"
#include "broker/broker.h"
#include "localization.h"

void Settings::AppConfig::postUpdate() {
    if (localization.wasUpdated()) {
        Localization::instance()->applyInfo(localization);
    }
    if (broker.wasUpdated()) {
        Radapter::Broker::instance()->applySettings(broker);
    }
}

void Settings::Modbus::postUpdate()
{
    for (auto [device, regs]: registers) {
        regs.init(device);
    }
    for (auto &slave: slaves) {
        slave.init();
    }
    for (auto &master: masters) {
        master.init();
    }
}
