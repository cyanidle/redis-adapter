#ifndef ROUTED_OBJECT_H
#define ROUTED_OBJECT_H

#include "jsonroute.h"
#include "serializable/serializable.h"
#include "serializable/bindable.hpp"
#include "serializable/validated.hpp"
#include "serializable/common_validators.h"

class RoutedObject : protected Serializable::Object {
public:
    RoutedObject(const QString &bindingName, const JsonRoute::KeysFilter &keysFilter = {});
    RoutedObject(const JsonRoute &binding, const JsonRoute::KeysFilter &keysFilter = {});
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
using RoutedBind = Serializable::Bindable<Serializable::Plain<T>>;
template <typename T>
using Routed = Serializable::Plain<T>;
template <typename T>
using RoutedSeq = Serializable::Sequence<T>;
using RoutedMinutes = Serializable::Validated<Serializable::Plain<int>>::With<Validator::Minutes>;
using RoutedHours = Serializable::Validated<Serializable::Plain<int>>::With<Validator::Hours24>;

#endif // BINDABLE_H
