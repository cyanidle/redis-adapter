#ifndef SETTINGS_SERIALIZABLESETTINGS_H
#define SETTINGS_SERIALIZABLESETTINGS_H

#include <QJsonDocument>
#include "serializable/serializable.h"
#include "serializable/validated.hpp"
#include "validators/validator_fetch.h"
#define NON_REQUIRED_ATTR "non_required"
Q_DECLARE_METATYPE(QJsonDocument::JsonFormat)

namespace Settings {

template<typename Target>
struct MarkNonRequired : public Target {
    FIELD_SUPER(Target)
    const QStringList &attributes() const {
        static const QStringList attrs{Target::attributes() + QStringList{NON_REQUIRED_ATTR}};
        return attrs;
    }
};

template<typename T>
using Required = Serializable::Plain<T>;
template<typename T>
using NonRequired = MarkNonRequired<Serializable::Plain<T>>;
template<typename T>
using RequiredSequence = Serializable::Sequence<T>;
template<typename T>
using NonRequiredSequence = MarkNonRequired<Serializable::Sequence<T>>;
template<typename T>
using RequiredMapping = Serializable::Mapping<T>;
template<typename T>
using NonRequiredMapping = MarkNonRequired<Serializable::Mapping<T>>;


using RequiredValidator = Serializable::Plain<Serializable::Validator>;
using NonRequiredValidator = MarkNonRequired<RequiredValidator>;

struct SerializableSettings : public Serializable::Object
{
    Q_GADGET
public:
    QString print() const;
    virtual bool update(const QVariantMap &src) override;
};

} // namespace Settings
#endif // SETTINGS_SERIALIZABLESETTINGS_H
