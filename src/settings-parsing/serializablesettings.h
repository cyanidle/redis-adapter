#ifndef SETTINGS_SERIALIZABLESETTINGS_H
#define SETTINGS_SERIALIZABLESETTINGS_H

#include <QJsonDocument>
#include "serializable/serializable.h"
#include "serializable/validated.hpp"
#include "validators/common_validators.h"
#include "validators/validator_fetch.h"

#define HAS_DEFAULT_ATTR "has_default"
#define OPTION_ATTR "option"

Q_DECLARE_METATYPE(QJsonDocument::JsonFormat)

namespace Settings {

template<typename Super>
struct MarkHasDefault : public Super {
    FIELD_SUPER(Super)
    const QStringList &attributes() const {
        static const QStringList attrs{Super::attributes() + QStringList{HAS_DEFAULT_ATTR}};
        return attrs;
    }
};

template <typename Super>
struct Option : Super {
    FIELD_SUPER(Super)
    bool updateWithVariant(const QVariant &source) {
        return m_valid = Super::updateWithVariant(source);
    }
    QVariant readVariant() const {
        if (!isValid()) {
            return {};
        } else {
            return Super::readVariant();
        }
    }
    const QStringList &attributes() const {
        static QStringList attrs{Super::attributes() + QStringList{OPTION_ATTR}};
        return attrs;
    }
    bool isValid() const {
        return m_valid;
    }
private:
    bool m_valid{false};
};

template<typename T>
using Required = Serializable::Plain<T>;
template<typename T>
using HasDefault = MarkHasDefault<Serializable::Plain<T>>;
template<typename T>
using Optional = Option<Required<T>>;
template<typename T>
using RequiredSequence = Serializable::Sequence<T>;
template<typename T>
using SequenceHasDefault = MarkHasDefault<Serializable::Sequence<T>>;
template<typename T>
using OptionalSequence = Option<RequiredSequence<T>>;
template<typename T>
using RequiredMapping = Serializable::Mapping<T>;
template<typename T>
using MappingHasDefault = MarkHasDefault<Serializable::Mapping<T>>;
template<typename T>
using OptionalMapping = Option<RequiredMapping<T>>;

using RequiredValidator = Serializable::Plain<Validator::Fetched>;
using OptionalValidator = Option<RequiredValidator>;

using RequiredLogLevel = Serializable::Validated<Required<QtMsgType>>::With<Validator::LogLevel>;
using NonRequiredLogLevel = MarkHasDefault<RequiredLogLevel>;

struct SerializableSettings : public Serializable::Object
{
    Q_GADGET
public:
    QString print() const;
    virtual bool update(const QVariantMap &src) override;
    void allowExtra(bool state = true);
    SerializableSettings();
protected:
    void checkForExtra(const QVariantMap &src);
    void processField(const QString &name, const QVariant &newValue);

    bool m_allowExtra;
    QString m_currentField;
};

} // namespace Settings
#endif // SETTINGS_SERIALIZABLESETTINGS_H
