#ifndef MODBUS_MASTER_H
#define MODBUS_MASTER_H

#include "broker/worker/worker.h"
#include "settings/modbussettings.h"
#include <QObject>

namespace Modbus {

class Master : public Radapter::Worker
{
    Q_OBJECT
public:
    Master();
};

} // namespace Modbus

#endif // MODBUS_MASTER_H
