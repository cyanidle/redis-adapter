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

template<typename User, typename UnderlyingValue>
using BindCallback = void (User::*)(UnderlyingValue &);

template<typename Target, BindCallback<Target, typename Target::valueType>...callbacks>
struct Bindable : protected Private::BindableSignals, public Target {
    using typename Target::valueType;
    using typename Target::valueRef;
    template<typename User>
    Bindable(User *user) {
        helpBind(user, callbacks...);
    }
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
protected:
    template<typename User, typename Slot>
    void helpBind(const User *who, Slot *what) {
        onUpdate(who, what);
    }
    template<typename User, typename Slot, typename...Slots>
    void helpBind(const User *who, Slot *what, Slots*...rest) {
        helpBind(who, what);
        helpBind(who, rest...);
    }
};

}

#endif // BINDABLE_HPP
