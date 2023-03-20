#ifndef BINDABLE_HPP
#define BINDABLE_HPP

#include <QObject>
#include <QSet>

namespace Serializable {
namespace Private {
struct BindableSignals : public QObject {
    Q_OBJECT
signals:
    void wasUpdated();
    void updateFailed();
};
}

template<typename Target>
struct Bindable : protected Private::BindableSignals, public Target {
    using typename Target::valueType;
    using typename Target::valueRef;
    using Target::Target;
    using Target::operator=;
    virtual const QStringList &attributes() const override {
        static const QStringList attrs = Target::attributes() + QStringList{"bindable"};
        return attrs;
    }
    template <typename...Args>
    QMetaObject::Connection onUpdate(Args&&...args) {
        return connect(this, &Private::BindableSignals::wasUpdated, std::forward<Args>(args)...);
    }
    QMetaObject::Connection onUpdate(void(*cb)(valueRef)) {
        return connect(this, &Private::BindableSignals::wasUpdated, [&](){cb(this->value);});
    }
    bool updateWithVariant(const QVariant &source) override {
        auto status = Target::updateWithVariant(source);
        if (status) {
            emit wasUpdated();
        } else {
            emit updateFailed();
        }
        return status;
    }
};

}

#endif // BINDABLE_HPP
