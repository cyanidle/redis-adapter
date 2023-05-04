#ifndef VALIDATED_FIELD_H
#define VALIDATED_FIELD_H

#include <QObject>
#include <QVariant>
#include <QSet>
#include "serializable/common_fields.hpp"
#include "serializable/field_super.h"
#define PRE_VALIDATER_ATTR "pre_validated"
namespace Serializable {

template <typename Validator>
void validate(QVariant &target) {
    static_assert(std::is_same<bool, decltype(Validator::validate(target))>(),
            "Validators must implement 'static bool validate(QVariant& value)' -> True = ok / False = invalid");
    static_assert(std::is_same<const QString &, decltype(Validator::name())>(),
            "Validators must implement 'static const QString &name()'");
    if (!target.isValid()) return;
    auto copy = target;
    if (!Validator::validate(target)) {
        reWarn()<<"Value:"<<copy<<"invalidated by:"<<Validator::name();
        target.clear();
    }
}

template <typename Validator, typename...Validators>
std::enable_if_t<sizeof...(Validators)>
validate(QVariant &target) {
    validate<Validator>(target);
    validate<Validators...>(target);
}


template <typename Validator, typename...Validators>
QString joinedNames() {
    if constexpr (!sizeof...(Validators)) {
        return Validator::name();
    } else {
        return Validator::name()+QStringLiteral(" + ")+joinedNames<Validators...>();
    }
}

template <typename Super, typename Validator, typename...Validators>
struct PreValidator : public Super {
    FIELD_SUPER(Super)
protected:
    using ft = ::Serializable::FieldType;
    enum {
        type = static_cast<ft>(Super::thisFieldType),
        is_sequence = type == ft::FieldSequence || type == ft::FieldSequenceOfNested,
        is_mapping = type == ft::FieldSequence || type == ft::FieldSequenceOfNested,
        is_plain = type == ft::FieldPlain || type == ft::FieldNested,
    };
    bool updateWithVariant(const QVariant &source) {
        if constexpr (is_sequence) {
            auto asList = source.toList();
            for (auto &val: asList) {
                validate<Validator, Validators...>(val);
                if (!val.isValid()) {
                    return Super::updateWithVariant(QVariant{});
                }
            }
            return Super::updateWithVariant(asList);
        } else if constexpr (is_mapping) {
            auto asMap = source.toMap();
            for (auto &val: asMap) {
                validate<Validator, Validators...>(val);
                if (!val.isValid()) {
                    return Super::updateWithVariant(QVariant{});
                }
            }
            return Super::updateWithVariant(asMap);
        } else {
            auto copy = source;
            validate<Validator, Validators...>(copy);
            return Super::updateWithVariant(copy);
        }
    }
    const QStringList &attributes() const {
        static const QStringList attrs = Super::attributes() + QStringList{PRE_VALIDATER_ATTR};
        return attrs;
    }
    const QString &typeName() const {
        static QString res = Super::typeName()+QStringLiteral(" --> ")+joinedNames<Validator, Validators...>();
        return res;
    }
};

template <typename Target>
struct Validated {
    template <typename...Validators>
    using With = PreValidator<Target, Validators...>;
};

template <typename Target, typename...Validators>
using Validate = PreValidator<Target, Validators...>;

#define VALIDATED(T, ...) \
::Serializable::Validate<T, __VA_ARGS__>

}

#endif // VALIDATED_FIELD_H
