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
protected:
    virtual const QStringList &attributes() const override {
        static const QStringList attrs{Target::attributes() + QStringList{"non_required"}};
        return attrs;
    }
};

template<typename T>
using NonRequiredSequence = MarkNonRequired<Serializable::Sequence<T>>;
template<typename T>
using NonRequiredField = MarkNonRequired<Serializable::Field<T>>;
template<typename T>
using RequiredSequence = Serializable::Sequence<T>;
template<typename T>
using RequiredField = Serializable::Field<T>;

struct SerializableSettings : public Serializable::Gadget
{
    Q_GADGET
public:
    QString print() const;
    virtual bool update(const QVariantMap &src) override;
};

} // namespace Settings

#endif // SETTINGS_SERIALIZABLESETTINGS_H
