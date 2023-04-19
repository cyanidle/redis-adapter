#ifndef MOCKWORKERSETTINGS_H
#define MOCKWORKERSETTINGS_H

#include "workersettings.h"
#include "private/global.h"

namespace Settings {

struct RADAPTER_API MockWorker : Worker
{
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(HasDefault<quint32>, mock_timer_delay, 3000)
    FIELD(HasDefault<QString>, json_file_path)
};

}

#endif // MOCKWORKERSETTINGS_H
