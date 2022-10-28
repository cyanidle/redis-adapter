#include "radapterschemes.h"

namespace Radapter {


AcknowledgeScheme * AcknowledgeScheme::m_instance;
RequestJsonCommand * RequestJsonCommand::m_instance;
RequestKeysScheme * RequestKeysScheme::m_instance;


Formatters::JsonDict AcknowledgeScheme::prepareMsg(const QVariant &source) const
{
    return Formatters::JsonDict{{"__ack__", source}};
}

QVariant AcknowledgeScheme::receive(const Formatters::JsonDict &source) const
{
    auto result = source["__ack__"].toMap();
    if (result.isEmpty()) {
        return QVariant();
    } else {
        return result;
    }
}

Formatters::JsonDict RequestJsonCommand::prepareMsg(const QVariant &source) const
{
    Q_UNUSED(source);
    return Formatters::JsonDict{{"__req_json__", true}};
}

QVariant RequestJsonCommand::receive(const Formatters::JsonDict &source) const
{
    auto result = source["__req_json__"].value<bool>();
    if (!result) {
        return QVariant();
    } else {
        return result;
    }
}

Formatters::JsonDict RequestKeysScheme::prepareMsg(const QVariant &source) const
{
    return Formatters::JsonDict{{"__req_keys__", source.toList()}};
}

QVariant RequestKeysScheme::receive(const Formatters::JsonDict &source) const
{
    auto result = source["__req_json__"].toList();
    if (result.isEmpty()) {
        return QVariant();
    } else {
        return result;
    }
}

} // namespace Radapter
