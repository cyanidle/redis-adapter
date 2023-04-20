#ifndef BINDABLE_HPP
#define BINDABLE_HPP

#include <QObject>
#include <QSet>

namespace Serializable {
struct Object;
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
    using Target::operator==;
    template <typename...Args>
    QMetaObject::Connection onUpdate(Args&&...args) {
        return connect(this, &Private::BindableSignals::wasUpdated, std::forward<Args>(args)...);
    }
    template<typename User>
    QMetaObject::Connection onUpdate(User *user, void(User::*cb)(valueRef)) {
        return connect(this, &Private::BindableSignals::wasUpdated, user, [&](){(user->*cb)(this->value);});
    }
    QMetaObject::Connection onUpdate(void(*cb)(valueRef)) {
        return connect(this, &Private::BindableSignals::wasUpdated, [&](){cb(this->value);});
    }
    const QStringList &attributes() const {
        static const QStringList attrs = Target::attributes() + QStringList{"bindable"};
        return attrs;
    }
    bool updateWithVariant(const QVariant &source) {
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
