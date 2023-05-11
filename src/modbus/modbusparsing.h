#ifndef MODBUS_PARSING_H
#define MODBUS_PARSING_H

#include "settings/modbussettings.h"

namespace Modbus {

QVariant parseModbusType(quint16* words, const Settings::RegisterInfo &regInfo, int sizeWords);
QModbusDataUnit parseValueToDataUnit(const QVariant &src, const Settings::RegisterInfo &regInfo);

QList<QModbusDataUnit> mergeDataUnits(const QList<QModbusDataUnit> &src);

QString printTable(QModbusDataUnit::RegisterType type);
QString printUnit(const QModbusDataUnit &unit);

}

#endif
