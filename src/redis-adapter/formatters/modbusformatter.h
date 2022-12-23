#ifndef MODBUSFORMATTER_H
#define MODBUSFORMATTER_H

#include <QObject>
#include "JsonFormatters"

class RADAPTER_SHARED_SRC ModbusFormatter : public QObject
{
    Q_OBJECT
public:
    explicit ModbusFormatter(const Formatters::Dict &jsonDict, QObject *parent = nullptr);

    Formatters::Dict toModbusUnit() const;
signals:

private:
    Formatters::Dict arrangeArrays(const Formatters::JsonDict &jsonDict) const;

    Formatters::Dict m_jsonDict;
};

#endif // MODBUSFORMATTER_H
