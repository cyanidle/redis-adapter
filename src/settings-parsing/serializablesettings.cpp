#include "serializablesettings.h"
#include "templates/algorithms.hpp"

namespace Settings {

SerializableSettings::SerializableSettings()
{

}

SerializableSettings *SerializableSettings::parent()
{
    return Serializer::Serializable::parent() ? Serializer::Serializable::parent()->as<SerializableSettings>() : nullptr;
}

const SerializableSettings *SerializableSettings::parent() const
{
    return Serializer::Serializable::parent() ? Serializer::Serializable::parent()->as<SerializableSettings>() : nullptr;
}

void SerializableSettings::deserialize(const QVariantMap &src)
{
    Serializer::SerializableGadget::deserialize(src);
    QList<SerializableSettings*> children;
    for (auto &field : fields()) {
        if (auto nested = getNested(field)) {
            throwOnNonSettings(nested);
            nested->as<SerializableSettings>()->updatePrefix(field);
            children.append(nested->as<SerializableSettings>());
        } else if (fieldType(field) == ContainerOfNested) {
            auto list = getNestedInContainer(field);
            for (auto nested : Radapter::enumerate(list)) {
                throwOnNonSettings(nested.value);
                nested.value->as<SerializableSettings>()->updatePrefix(field + ":" + QString::number(nested.count));
                children.append(nested.value->as<SerializableSettings>());
            }
        } else if (fieldType(field) == MapOfNested) {
            auto map = getNestedInMap(field);
            for (auto nested{map.begin()}; nested != map.end(); ++nested) {
                throwOnNonSettings(nested.value());
                nested.value()->as<SerializableSettings>()->updatePrefix(field + ":" + nested.key());
                children.append(nested.value()->as<SerializableSettings>());
            }
        }
    }
    if (!parent()) {
        for (auto &child : children) {
            child->performCheck();
        }
        performCheck();
    }
}

QString SerializableSettings::prefix() const
{
    QString result = m_prefix;
    auto nextParent = parent();
    while (nextParent) {
        result = nextParent->prefix() + "/" + result;
        nextParent = nextParent->parent();
    }
    return result;
}

void SerializableSettings::performCheck()
{
    for (auto &field : fields()) {
        if (!fieldHasDefault(field) && !wasUpdated(field)) {
            throw std::runtime_error("Settings Deserialization error! --> " + prefix().toStdString() + "/" + field.toStdString());
        }
    }
}

void SerializableSettings::throwOnNonSettings(const Serializable *child) const
{
    if (!child->is<SerializableSettings>()) {
        auto msg = prefix() + ":" + "SerializableSettings settings must contain only other SerializableSettings subclasses!";
        throw std::runtime_error(msg.toStdString());
    }
}

void SerializableSettings::updatePrefix(const QString &name)
{
    m_prefix = name;
}

} // namespace Settings
