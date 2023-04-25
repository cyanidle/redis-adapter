#ifndef SETTINGS_SERIALIZABLESETTINGS_H
#define SETTINGS_SERIALIZABLESETTINGS_H

#include <QJsonDocument>
#include "serializable/serializableobject.h"
#include "serializable/validated.hpp"
#include "validators/common_validators.h"
#include "validators/validator_fetch.h"

#define HAS_DEFAULT_ATTR "has_default"
#define OPTION_ATTR "optional"

Q_DECLARE_METATYPE(QJsonDocument::JsonFormat)

namespace Settings {

template<typename Super>
struct MarkHasDefault : public Super {
    FIELD_SUPER(Super)
protected:
    const QStringList &attributes() const {
        static const QStringList attrs{Super::attributes() + QStringList{HAS_DEFAULT_ATTR}};
        return attrs;
    }
};

template <typename Super>
struct MarkOptional : Super {
    FIELD_SUPER(Super)
    bool isValid() const {
        return m_valid;
    }
protected:
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
private:
    bool m_valid{false};
};

template<typename T> using Required = Serializable::Plain<T>;
template<typename T> using HasDefault = MarkHasDefault<Serializable::Plain<T>>;
template<typename T> using Optional = MarkOptional<Required<T>>;
template<typename T> using RequiredSequence = Serializable::Sequence<T>;
template<typename T> using SequenceHasDefault = MarkHasDefault<Serializable::Sequence<T>>;
template<typename T> using OptionalSequence = MarkOptional<RequiredSequence<T>>;
template<typename T> using RequiredMapping = Serializable::Mapping<T>;
template<typename T> using MappingHasDefault = MarkHasDefault<Serializable::Mapping<T>>;
template<typename T> using OptionalMapping = MarkOptional<RequiredMapping<T>>;

using RequiredValidator = Serializable::Plain<Validator::Fetched>;
using OptionalValidator = MarkOptional<RequiredValidator>;

using RequiredLogLevel = Serializable::Validated<Required<QtMsgType>>::With<Validator::LogLevel>;
using NonRequiredLogLevel = MarkHasDefault<RequiredLogLevel>;


struct Serializable : public ::Serializable::Object
{
    Q_GADGET
public:
    QString print() const;
    virtual bool update(const QVariantMap &src) override;
    QVariantMap printExample() const;
    void allowExtra(bool state = true);
    Serializable();
protected:
    QVariant printExample(const ::Serializable::FieldConcept *field) const;
    void checkForExtra(const QVariantMap &src);
    void processField(const QString &name, const QVariant &newValue);

    QString m_currentField;
    bool m_allowExtra;
private:
    QVariant printExamplePlain(const ::Serializable::FieldConcept *field) const;
    QVariant printExampleNested(const ::Serializable::FieldConcept *field) const;
};

} // namespace Settings
#endif // SETTINGS_SERIALIZABLESETTINGS_H
