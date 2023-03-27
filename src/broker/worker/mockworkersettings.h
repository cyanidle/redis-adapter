#ifndef MOCKWORKERSETTINGS_H
#define MOCKWORKERSETTINGS_H

#include "workersettings.h"
#include "private/global.h"

namespace Radapter {
struct RADAPTER_API MockWorkerSettings;
}

struct Radapter::MockWorkerSettings : WorkerSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Settings::NonRequired<quint32>, mock_timer_delay)
    FIELD(Settings::NonRequired<QString>, json_file_path)
};

#endif // MOCKWORKERSETTINGS_H
