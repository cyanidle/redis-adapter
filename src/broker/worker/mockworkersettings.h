#ifndef MOCKWORKERSETTINGS_H
#define MOCKWORKERSETTINGS_H

#include "workersettings.h"
#include "private/global.h"

namespace Radapter {
struct RADAPTER_SHARED_SRC MockWorkerSettings;
}

struct Radapter::MockWorkerSettings : WorkerSettings {
    Q_GADGET
    IS_SERIALIZABLE
    SERIAL_FIELD(quint32, mock_timer_delay, 3000);
    SERIAL_FIELD(QString, json_file_path, "")
};

#endif // MOCKWORKERSETTINGS_H
