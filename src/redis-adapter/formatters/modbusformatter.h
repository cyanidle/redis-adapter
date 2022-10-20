#ifndef MODBUSFORMATTER_H
#define MODBUSFORMATTER_H

#include <QObject>
#include <json-formatters/formatters/dict.h>
#include <json-formatters/formatters/list.h>

class RADAPTER_SHARED_SRC ModbusFormatter : public QObject
{
    Q_OBJECT
public:
    explicit ModbusFormatter(const Formatters::Dict &jsonDict, QObject *parent = nullptr);

    Formatters::Dict toModbusUnit() const;
signals:

private:
    Formatters::Dict arrangeArrays(const Formatters::Dict &jsonDict) const;

    Formatters::Dict m_jsonDict;
};

#endif // MODBUSFORMATTER_H
