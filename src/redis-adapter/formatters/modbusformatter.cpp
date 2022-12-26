#include "modbusformatter.h"
#include "timeformatter.h"

ModbusFormatter::ModbusFormatter(const JsonDict &jsonDict, QObject *parent)
    : QObject(parent),
      m_jsonDict(jsonDict)
{
}

 JsonDict ModbusFormatter::toModbusUnit() const
{
    if (m_jsonDict.isEmpty()) {
        return JsonDict{};
    }

    auto jsonUnit = arrangeArrays(m_jsonDict);
    auto modbusUnit = jsonUnit.nest();
    return modbusUnit;
}

 JsonDict ModbusFormatter::arrangeArrays(const JsonDict &jsonDict) const
{
    return jsonDict.flatten(".");
}
