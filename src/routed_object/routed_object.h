#ifndef ROUTED_OBJECT_H
#define ROUTED_OBJECT_H

#include "jsonroute.h"
#include "serializable/serializable.h"
#include "serializable/bindable.hpp"
#include "serializable/validated.hpp"
#include "serializable/common_validators.hpp"

class RoutedBase : protected Serializable::Object {
public:
    RoutedBase(const QString &bindingName, const JsonRoute::KeysFilter &keysFilter);
    RoutedBase(const JsonRoute &binding, const JsonRoute::KeysFilter &keysFilter);
    void routedUpdate(const JsonDict &data);
    const QStringList mappedFields(const QString& separator = ":") const;
    const QStringList mappedFieldsExclude(const QStringList &exclude, const QString& separator = ":") const;
    const QString mappedField(const QString &field, const QString& separator = ":") const;
    JsonDict send(const QString &fieldName = {}) const;
    JsonDict sendGlob(const QString &glob) const;
    QString print() const;
    bool wasUpdated(const QString &fieldName) const;
    bool wasUpdatedGlob(const QString &glob) const;
    bool isIgnored(const QString &fieldName) const;
    void checkIfOk() const;
private:
    bool update(const QVariantMap &src) override;

    JsonRoute m_binding;
    QList<QString> m_wereUpdated;
    mutable bool m_checkPassed{false};
};

template <typename T>
using RoutedBind = Serializable::Bindable<Serializable::Field<T>>;
template <typename T>
using Routed = Serializable::Field<T>;
template <typename T>
using RoutedSeq = Serializable::Sequence<T>;
using RoutedMinutes = Serializable::Validated<Serializable::Field<bool>>::With<Validate::Minutes>;
using RoutedHours = Serializable::Validated<Serializable::Field<bool>>::With<Validate::Hours24>;

struct RoutedGadget : public Serializable::GadgetMixin<RoutedBase> {
    RoutedGadget(const QString &bindingName, const JsonRoute::KeysFilter &keysFilter = {});
    RoutedGadget(const JsonRoute &binding, const JsonRoute::KeysFilter &keysFilter = {});
};

class RoutedQObject : public Serializable::QObjectMixin<RoutedBase> {
    Q_OBJECT
    Q_DISABLE_COPY(RoutedQObject)
public:
    RoutedQObject(const QString &bindingName, const JsonRoute::KeysFilter &keysFilter = {}, QObject *parent = nullptr);
    RoutedQObject(const JsonRoute &binding, const JsonRoute::KeysFilter &keysFilter = {}, QObject *parent = nullptr);
public slots:
    void routedUpdate(const JsonDict &data);
};

#endif // BINDABLE_H
