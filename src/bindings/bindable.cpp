#include "bindable.h"
#include "bindings/bindingsprovider.h"

Bindable::Bindable(const QString &bindingName, const JsonBinding::KeysFilter &keysFilter) :
    Bindable(BindingsProvider::instance()->getBinding(bindingName), keysFilter)
{}

Bindable::Bindable(const JsonBinding &binding, const JsonBinding::KeysFilter &keysFilter) :
    m_binding(binding.optimise(keysFilter))
{
}

BindableGadget::BindableGadget(const JsonBinding &binding, const JsonBinding::KeysFilter &keysFilter) :
    GadgetMixin(binding, keysFilter)
{

}

BindableGadget::BindableGadget(const QString &bindingName, const JsonBinding::KeysFilter &keysFilter) :
    GadgetMixin(BindingsProvider::instance()->getBinding(bindingName), keysFilter)
{

}

BindableQObject::BindableQObject(const QString &bindingName, const JsonBinding::KeysFilter &keysFilter, QObject *parent) :
    BindableQObject(BindingsProvider::instance()->getBinding(bindingName), keysFilter, parent)
{

}

BindableQObject::BindableQObject(const JsonBinding &binding, const JsonBinding::KeysFilter &keysFilter, QObject *parent) :
    QObjectMixin(parent, binding, keysFilter)
{

}

void BindableQObject::update(const JsonDict &data)
{
    Bindable::update(data);
}

void Bindable::update(const JsonDict &data)
{
    checkIfOk();
    auto result = m_binding.receive(data);
    if (!result.isEmpty()) this->deserialize(result.data(), false);
}

const QStringList Bindable::mappedFields(const QString &separator) const
{
    return send().flatten(separator).keys();
}

JsonDict Bindable::send(const QString &fieldName) const
{
    checkIfOk();
    if (fieldName.isEmpty()) return m_binding.send(this->serialize());
    else return m_binding.send(serializeField(fieldName));
}

bool Bindable::isIgnored(const QString &fieldName) const
{
    return m_binding.ignoredFields().contains(fieldName);
}

void Bindable::checkIfOk() const
{
    if (m_checkPassed) {
        return;
    }
    auto structure = JsonDict{this->structure()};
    for (auto &iter : structure) {
        try {
            m_binding.requireValueName(iter.key().constFirst());
        } catch (std::exception &e){
            brokerError() << "Required: " << structure.topKeys();
            throw std::runtime_error(e.what());
        }
    }
    m_checkPassed = true;
}
