#include "bindingsprovider.h"
#include "radapterlogging.h"

BindingsProvider::BindingsProvider(const JsonBinding::Map &bindings, QObject* parent) :
    QObject(parent),
    m_bindings(bindings)
{
}

const JsonBinding &BindingsProvider::getBinding(const QString &name)
{
    if (m_bindings.contains(name)) {
        return m_bindings[name];
    } else {
        throw std::runtime_error(std::string("Missing binding: ") + name.toStdString());
    }
}
