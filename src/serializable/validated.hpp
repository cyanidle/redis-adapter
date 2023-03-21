#ifndef VALIDATED_H
#define VALIDATED_H

#include <QObject>
#include <QVariant>
#include <QSet>

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
    using typename Target::valueType;
    using typename Target::valueRef;
    using Target::Target;
    using Target::operator=;
    using Target::operator==;
protected:
    bool updateWithVariant(const QVariant &source) override {
        auto copy = source;
        validate<Validator, Validators...>(copy);
        return Target::updateWithVariant(copy);
    }
    virtual const QStringList &attributes() const override {
        static const QStringList attrs = Target::attributes() + QStringList{"pre_validated"};
        return attrs;
    }
};

template <typename Target>
struct Validated {
    template <typename...Validators>
    using With = PreValidator<Target, Validators...>;
};

template <typename Target, typename...Validators>
using Validate = typename Validated<Target>::template With<Validators...>;

}

#endif // VALIDATED_H
