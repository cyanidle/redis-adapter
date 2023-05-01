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
void validate(QVariant &target, const QVariantList &args, QVariant &state) {
    static_assert(std::is_same<bool, decltype(Validator::validate(target, args, state))>(),
            "Validators must implement 'static bool validate(QVariant& value, const QVariantList &args, QVariant &state)'. True = ok / False = invalid");
    if (!target.isValid()) return;
    if (!Validator::validate(target, args, state)) {
        target = QVariant();
    }
}

template <typename Validator, typename...Validators>
void validate(QVariant &target, const QVariantList &args, QVariant &state, typename std::enable_if<sizeof...(Validators)>::type* = nullptr) {
    validate<Validator>(target, args, state);
    validate<Validators...>(target, args, state);
}

template <typename Target, typename Validator, typename...Validators>
struct PreValidator : public Target {
    FIELD_SUPER(Target)
    bool updateWithVariant(const QVariant &source) {
        auto copy = source;
        auto nullState = QVariant{};
        validate<Validator, Validators...>(copy, {}, nullState);
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

}

#endif // VALIDATED_H
