#ifndef BINDABLE_H
#define BINDABLE_H

#include "bindings/jsonbinding.h"
#include "settings-parsing/serializer.hpp"

class Bindable : protected Serializer::Serializable {
public:
    Bindable(const QString &bindingName, const JsonBinding::KeysFilter &keysFilter);
    Bindable(const JsonBinding &binding, const JsonBinding::KeysFilter &keysFilter);
    void update(const JsonDict &data);
    const QStringList mappedFields(const QString& separator = ":") const;
    const QStringList mappedFieldsExclude(const QStringList &exclude, const QString& separator = ":") const;
    const QString mappedField(const QString &field, const QString& separator = ":") const;
    JsonDict send(const QString &fieldName = {}) const;
    JsonDict sendGlob(const QString &glob) const;
    bool isIgnored(const QString &fieldName) const;
    void checkIfOk() const;
private:
    JsonBinding m_binding;
    mutable bool m_checkPassed{false};
};

struct BindableGadget : public Serializer::GadgetMixin<Bindable> {
    BindableGadget(const QString &bindingName, const JsonBinding::KeysFilter &keysFilter = {});
    BindableGadget(const JsonBinding &binding, const JsonBinding::KeysFilter &keysFilter = {});
};

class BindableQObject : public Serializer::QObjectMixin<Bindable> {
    Q_OBJECT
    Q_DISABLE_COPY(BindableQObject)
public:
    BindableQObject(const QString &bindingName, const JsonBinding::KeysFilter &keysFilter = {}, QObject *parent = nullptr);
    BindableQObject(const JsonBinding &binding, const JsonBinding::KeysFilter &keysFilter = {}, QObject *parent = nullptr);
public slots:
    void update(const JsonDict &data);
};

#endif // BINDABLE_H
