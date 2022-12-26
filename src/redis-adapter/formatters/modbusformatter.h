#ifndef MODBUSFORMATTER_H
#define MODBUSFORMATTER_H

#include <QObject>
#include "jsondict/jsondict.h"

class RADAPTER_SHARED_SRC ModbusFormatter : public QObject
{
    Q_OBJECT
public:
    explicit ModbusFormatter(const JsonDict &jsonDict, QObject *parent = nullptr);

    JsonDict toModbusUnit() const;
signals:

private:
    JsonDict arrangeArrays(const JsonDict &jsonDict) const;

    JsonDict m_jsonDict;
};

#endif // MODBUSFORMATTER_H
