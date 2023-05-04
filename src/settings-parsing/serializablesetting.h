#ifndef SETTINGS_SERIALIZABLESETTINGS_H
#define SETTINGS_SERIALIZABLESETTINGS_H

#include <QJsonDocument>
#include "serializable/serializableobject.h"
#include "validators/validated_field.hpp"
#include "validators/common_validators.h"
#include "private/impl_settingscomment.h"
#include "validators/validator_fetch.h"
#include "settingsexample.h"

#define HAS_DEFAULT_ATTR "has_default"
#define OPTION_ATTR "optional"
#define REQUIRED_ATTR "required"

Q_DECLARE_METATYPE(QJsonDocument::JsonFormat)

namespace Settings {

template<typename Super>
struct MarkRequired : public Super {
    FIELD_SUPER(Super)
protected:
    const QStringList &attributes() const {
        static const QStringList attrs{Super::attributes() + QStringList{REQUIRED_ATTR}};
        return attrs;
    }
};

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
        static QStringList attrs = [this]{
            auto super = Super::attributes();
            super.removeAll(REQUIRED_ATTR);
            return super + QStringList{OPTION_ATTR};
        }();
        return attrs;
    }
private:
    bool m_valid{false};
};

template<typename T> using Required = MarkRequired<::Serializable::Plain<T>>;
template<typename T> using HasDefault = MarkHasDefault<::Serializable::Plain<T>>;
template<typename T> using Optional = MarkOptional<::Serializable::Plain<T>>;
template<typename T> using RequiredSequence = MarkRequired<::Serializable::Sequence<T>>;
template<typename T> using SequenceHasDefault = MarkHasDefault<::Serializable::Sequence<T>>;
template<typename T> using OptionalSequence = MarkOptional<::Serializable::Sequence<T>>;
template<typename T> using RequiredMapping = MarkRequired<::Serializable::Mapping<T>>;
template<typename T> using MappingHasDefault = MarkHasDefault<::Serializable::Mapping<T>>;
template<typename T> using OptionalMapping = MarkOptional<::Serializable::Mapping<T>>;
template<typename T> using Validated = ::Serializable::Validated<T>;
using RequiredValidator = Required<Validator::Fetched>;
using OptionalValidator = Optional<Validator::Fetched>;

struct Serializable : public ::Serializable::Object
{
    Q_GADGET
public:
    QString print() const;
    virtual bool update(const QVariantMap &src) override;
    QString getComment(const QString &fieldName) const;
    QString getClassComment() const;
    Example getExample() const;
    void allowExtra(bool state = true);
    Serializable();
protected:
    FieldExample getExample(const ::Serializable::FieldConcept *field) const;
    void checkForExtra(const QVariantMap &src);
    void processField(const QString &name, const QVariant &newValue);

    QString m_currentField;
    bool m_allowExtra;
private:
    FieldExample getExamplePlain(const ::Serializable::FieldConcept *field) const;
    FieldExample getExampleNested(const ::Serializable::FieldConcept *field) const;
};
#define IS_SETTING \
    IS_SERIALIZABLE_BASE
} // namespace Settings
#endif // SETTINGS_SERIALIZABLESETTINGS_H
