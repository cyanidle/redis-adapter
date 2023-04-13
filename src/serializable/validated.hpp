#ifndef VALIDATED_H
#define VALIDATED_H

#include <QObject>
#include <QVariant>
#include <QSet>
#include "field_super.h"
#define PRE_VALIDATER_ATTR "pre_validated"
namespace Serializable {

template <typename Validator>
void validate(QVariant &target) {
    static_assert(std::is_same<bool, decltype(Validator::validate(target))>(),
            "Validators must implement 'static bool validate(QVariant& value)'. True = ok / False = invalid");
    if (!target.isValid()) return;
    if (!Validator::validate(target)) {
        target = QVariant();
    }
}

template <typename Validator, typename...Validators>
void validate(QVariant &target, typename std::enable_if<sizeof...(Validators)>::type* = nullptr) {
    validate<Validator>(target);
    validate<Validators...>(target);
}

template <typename Target, typename Validator, typename...Validators>
struct PreValidator : public Target {
    FIELD_SUPER(Target)
    bool updateWithVariant(const QVariant &source) {
        auto copy = source;
        validate<Validator, Validators...>(copy);
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
