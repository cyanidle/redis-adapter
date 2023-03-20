#ifndef ROUTED_H
#define ROUTED_H

#include "bindings/jsonroute.h"
#include "serializable/serializable.h"

class Routed : protected Serializable::Object {
public:
    Routed(const QString &bindingName, const JsonRoute::KeysFilter &keysFilter);
    Routed(const JsonRoute &binding, const JsonRoute::KeysFilter &keysFilter);
    void routedUpdate(const JsonDict &data);
    const QStringList mappedFields(const QString& separator = ":") const;
    const QStringList mappedFieldsExclude(const QStringList &exclude, const QString& separator = ":") const;
    const QString mappedField(const QString &field, const QString& separator = ":") const;
    JsonDict send(const QString &fieldName = {}) const;
    JsonDict sendGlob(const QString &glob) const;
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

struct RoutedGadget : public Serializable::GadgetMixin<Routed> {
    RoutedGadget(const QString &bindingName, const JsonRoute::KeysFilter &keysFilter = {});
    RoutedGadget(const JsonRoute &binding, const JsonRoute::KeysFilter &keysFilter = {});
};

class RoutedQObject : public Serializable::QObjectMixin<Routed> {
    Q_OBJECT
    Q_DISABLE_COPY(RoutedQObject)
public:
    RoutedQObject(const QString &bindingName, const JsonRoute::KeysFilter &keysFilter = {}, QObject *parent = nullptr);
    RoutedQObject(const JsonRoute &binding, const JsonRoute::KeysFilter &keysFilter = {}, QObject *parent = nullptr);
public slots:
    void routedUpdate(const JsonDict &data);
};

#endif // BINDABLE_H
