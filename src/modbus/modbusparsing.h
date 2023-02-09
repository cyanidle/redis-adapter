#ifndef MODBUS_PARSING_H
#define MODBUS_PARSING_H

#include "settings/modbussettings.h"
namespace Modbus {

QVariant parseModbusType(quint16* words, const Settings::RegisterInfo &regInfo, int sizeWords, const Settings::PackingMode &endianess);
QModbusDataUnit parseValueToDataUnit(const QVariant &src, const Settings::RegisterInfo &regInfo, const Settings::PackingMode &endianess);
QList<QModbusDataUnit> mergeDataUnits(const QList<QModbusDataUnit> &src);

}

#endif
