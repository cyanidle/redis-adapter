#include "modbusformatter.h"
#include "timeformatter.h"

ModbusFormatter::ModbusFormatter(const Formatters::Dict &jsonDict, QObject *parent)
    : QObject(parent),
      m_jsonDict(jsonDict)
{
}

Formatters::Dict ModbusFormatter::toModbusUnit() const
{
    if (m_jsonDict.isEmpty()) {
        return Formatters::Dict{};
    }

    auto jsonUnit = arrangeArrays(m_jsonDict);
    auto modbusUnit = jsonUnit.nest();
    return modbusUnit;
}

Formatters::Dict ModbusFormatter::arrangeArrays(const Formatters::JsonDict &jsonDict) const
{
    return jsonDict.flatten(".");
}
