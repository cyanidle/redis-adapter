#ifndef MOCKWORKERSETTINGS_H
#define MOCKWORKERSETTINGS_H

#include "workersettings.h"
#include "private/global.h"

namespace Radapter {
struct RADAPTER_API MockWorkerSettings;
}

struct Radapter::MockWorkerSettings : WorkerSettings {
    Q_GADGET
    FIELDS(mock_timer_delay, json_file_path)
    Settings::NonRequired<Serializable::Field<quint32>> mock_timer_delay;
    Settings::NonRequired<Serializable::Field<QString>> json_file_path;
};

#endif // MOCKWORKERSETTINGS_H
