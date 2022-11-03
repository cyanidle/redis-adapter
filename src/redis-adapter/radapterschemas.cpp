#include "radapterschemas.h"

using namespace Radapter;

#define SCHEMA_FIELD "__schema__"
#define SCHEMA_DATA_FIELD "__schema_data__"

AcknowledgeSchema * AcknowledgeSchema::m_instance;
RequestJsonSchema * RequestJsonSchema::m_instance;
RequestKeysSchema * RequestKeysSchema::m_instance;


Formatters::JsonDict AcknowledgeSchema::prepareMsg(const QVariant &source) const
{
    auto result = Formatters::JsonDict(source);
    result[SCHEMA_FIELD] = "__ack__";
    return result;
}

QVariant AcknowledgeSchema::receive(const Formatters::JsonDict &source) const
{
    auto isAck = source[SCHEMA_FIELD].toString() == "__ack__";
    return isAck ? source.data() : QVariant();
}

Formatters::JsonDict RequestJsonSchema::prepareMsg(const QVariant &source) const
{
    Q_UNUSED(source)
    auto result = Formatters::JsonDict();
    result[SCHEMA_FIELD] = "__req_json__";
    result[SCHEMA_DATA_FIELD] = true;
    return result;
}

QVariant RequestJsonSchema::receive(const Formatters::JsonDict &source) const
{
    auto isReqJson = source[SCHEMA_FIELD].toString() == "__req_json__";
    return isReqJson ? source[SCHEMA_DATA_FIELD] : QVariant();
}

Formatters::JsonDict RequestKeysSchema::prepareMsg(const QVariant &source) const
{
    auto result = Formatters::JsonDict(source);
    result[SCHEMA_FIELD] = "__req_keys__";
    result[SCHEMA_DATA_FIELD] = source.toList();
    return result;
}

QVariant RequestKeysSchema::receive(const Formatters::JsonDict &source) const
{
    if (source[SCHEMA_FIELD].toString() != "__req_keys__") {
        return QVariant();
    }
    const auto temp = source[SCHEMA_DATA_FIELD].toList();
    QStringList result;
    for (const auto &key : temp) {
        result.append(key.toString());
    }
    return result.isEmpty() ? result : QVariant();
}

