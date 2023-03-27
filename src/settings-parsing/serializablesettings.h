#ifndef SETTINGS_SERIALIZABLESETTINGS_H
#define SETTINGS_SERIALIZABLESETTINGS_H

#include <QJsonDocument>
#include "serializable/serializable.h"
#include "serializable/validated.hpp"

Q_DECLARE_METATYPE(QJsonDocument::JsonFormat)

namespace Settings {

template<typename Target>
struct MarkNonRequired : public Target {
    using typename Target::valueType;
    using typename Target::valueRef;
    using Target::Target;
    using Target::operator=;
    using Target::operator==;
    const QStringList &attributes() const {
        static const QStringList attrs{Target::attributes() + QStringList{"non_required"}};
        return attrs;
    }
};

template<typename T>
using NonRequiredSequence = MarkNonRequired<Serializable::Sequence<T>>;
template<typename T>
using NonRequired = MarkNonRequired<Serializable::Plain<T>>;
template<typename T>
using RequiredSequence = Serializable::Sequence<T>;
template<typename T>
using Required = Serializable::Plain<T>;

struct SerializableSettings : public Serializable::Object
{
    Q_GADGET
public:
    QString print() const;
    virtual bool update(const QVariantMap &src) override;
};

} // namespace Settings

#endif // SETTINGS_SERIALIZABLESETTINGS_H
