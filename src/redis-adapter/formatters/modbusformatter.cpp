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

Formatters::Dict ModbusFormatter::arrangeArrays(const Formatters::Dict &jsonDict) const
{
    auto arraysFlattened = Formatters::Dict{};
    auto separator = ":";
    for (auto jsonItem = jsonDict.begin();
         jsonItem != jsonDict.end();
         jsonItem++)
    {
        auto levelKeys = jsonItem.key().split(separator);
        auto arrayIndex = levelKeys.takeLast();
        if (arrayIndex.toUInt() > 0u) {
            auto arrayName = QStringList{ levelKeys.takeLast(), arrayIndex }.join(".");
            auto arrayItem = Formatters::Dict{ QVariantMap{{ arrayName, jsonItem.value() }} };
            arraysFlattened.insert(levelKeys.join(separator), arrayItem);
        } else {
            arraysFlattened.insert(jsonItem.key(), jsonItem.value());
        }
    }
    return arraysFlattened;
}
