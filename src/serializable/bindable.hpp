#ifndef BINDABLE_HPP
#define BINDABLE_HPP

#include "common_fields.hpp"
#include <QObject>
#include <QSet>
#define BINDABLE_ATTR "bindable"
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
    FIELD_SUPER(Target)
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
protected:
    const QStringList &attributes() const {
        static const QStringList attrs = Target::attributes() + QStringList{BINDABLE_ATTR};
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
