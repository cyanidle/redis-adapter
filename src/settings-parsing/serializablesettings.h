#ifndef SETTINGS_SERIALIZABLESETTINGS_H
#define SETTINGS_SERIALIZABLESETTINGS_H

#include <QJsonDocument>
#include "serializable/serializable.h"
#include "serializable/validated.hpp"

Q_DECLARE_METATYPE(QJsonDocument::JsonFormat)

namespace Settings {

template<typename Target>
struct NonRequired : public Target {
    using typename Target::valueType;
    using typename Target::valueRef;
    using Target::Target;
    using Target::operator=;
    using Target::operator==;
protected:
    virtual const QStringList &attributes() const override {
        static const QStringList attrs = Target::attributes() + QStringList{"non_required"};
        return attrs;
    }
};

template<typename T>
using NonRequiredSeq = NonRequired<Serializable::Sequence<T>>;
template<typename T>
using NonRequiredField = NonRequired<Serializable::Field<T>>;
template<typename T>
using RequiredSeq = Serializable::Sequence<T>;
template<typename T>
using RequiredField = Serializable::Field<T>;

struct SerializableSettings : public Serializable::Gadget
{
    Q_GADGET
public:
    QString print() const;
    virtual bool update(const QVariantMap &src) override;
};

struct ChooseJsonFormat {
    static bool validate(QVariant &src) {
        static QMap<QString, QJsonDocument::JsonFormat> map {
            {"compact", QJsonDocument::Compact},
            {"indented", QJsonDocument::Indented}
        };
        auto asStr = src.toString().toLower();
        src.setValue(map.value(asStr));
        return map.contains(asStr);
    }
};

} // namespace Settings

#endif // SETTINGS_SERIALIZABLESETTINGS_H
