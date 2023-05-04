#ifndef BINDABLE_FIELD_H
#define BINDABLE_FIELD_H

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
    void beforeUpdate(const QVariant &source);
    void wasUpdated();
    void updateFailed();
};
}

template<typename Target>
struct Bindable : protected Private::BindableSignals, public Target {
    FIELD_SUPER(Target)
    template <typename...Args>
    QMetaObject::Connection afterUpdate(Args&&...args) {
        return connect(this, &Private::BindableSignals::wasUpdated, std::forward<Args>(args)...);
    }
    template<typename User>
    QMetaObject::Connection afterUpdate(User *user, void(User::*cb)(valueRef)) {
        return connect(this, &Private::BindableSignals::wasUpdated, user, [&](){(user->*cb)(this->value);});
    }
    QMetaObject::Connection afterUpdate(void(*cb)(valueRef)) {
        return connect(this, &Private::BindableSignals::wasUpdated, [&](){cb(this->value);});
    }
    template <typename...Args>
    QMetaObject::Connection beforeUpdate(Args&&...args) {
        return connect(this, &Private::BindableSignals::beforeUpdate, std::forward<Args>(args)...);
    }
    template<typename User>
    QMetaObject::Connection beforeUpdate(User *user, void(User::*cb)(valueRef, const QVariant &wanted)) {
        return connect(this, &Private::BindableSignals::wasUpdated, user, [&](const QVariant &wanted){
            (user->*cb)(this->value, wanted);
        });
    }
    QMetaObject::Connection beforeUpdate(void(*cb)(valueRef, const QVariant &wanted)) {
        return connect(this, &Private::BindableSignals::wasUpdated, [&](const QVariant &wanted){
            cb(this->value, wanted);
        });
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

#endif // BINDABLE_FIELD_H
