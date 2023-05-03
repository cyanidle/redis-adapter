#ifndef VALIDATED_H
#define VALIDATED_H

#include <QObject>
#include <QVariant>
#include <QSet>
#include "serializable/common_fields.hpp"
#include "field_super.h"
#define PRE_VALIDATER_ATTR "pre_validated"
namespace Serializable {

template <typename Validator>
void validate(QVariant &target) {
    static_assert(std::is_same<bool, decltype(Validator::validate(target))>(),
            "Validators must implement 'static bool validate(QVariant& value)' -> True = ok / False = invalid");
    if (!target.isValid()) return;
    if (!Validator::validate(target)) {
        target.clear();
    }
}

template <typename Validator, typename...Validators>
std::enable_if_t<sizeof...(Validators)>
validate(QVariant &target) {
    validate<Validator>(target);
    validate<Validators...>(target);
}

template <typename Target, typename Validator, typename...Validators>
struct PreValidator : public Target {
    FIELD_SUPER(Target)
protected:
    using ft = ::Serializable::FieldType;
    enum {
        type = static_cast<ft>(Target::thisFieldType),
        is_sequence = type == ft::FieldSequence || type == ft::FieldSequenceOfNested,
        is_mapping = type == ft::FieldSequence || type == ft::FieldSequenceOfNested,
        is_plain = type == ft::FieldPlain || type == ft::FieldNested,
    };
    bool updateWithVariant(const QVariant &source) {
        auto copy = source;
        if constexpr (is_sequence) {
            auto asList = copy.toList();
            for (auto &val: asList) {
                validate<Validator, Validators...>(val);
            }
            copy.setValue(asList);
        } else if constexpr (is_mapping) {
            auto asMap = copy.toMap();
            for (auto &val: asMap) {
                validate<Validator, Validators...>(val);
            }
            copy.setValue(asMap);
        } else {
            validate<Validator, Validators...>(copy);
        }
        return Target::updateWithVariant(copy);
    }
    const QStringList &attributes() const {
        static const QStringList attrs = Target::attributes() + QStringList{PRE_VALIDATER_ATTR};
        return attrs;
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

#endif // VALIDATED_H
