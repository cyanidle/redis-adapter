#include "routesprovider.h"
#include "radapterlogging.h"

JsonRoutesProvider::JsonRoutesProvider(const JsonRoute::Map &bindings, QObject* parent) :
    QObject(parent),
    m_bindings(bindings)
{
}

const JsonRoute &JsonRoutesProvider::getBinding(const QString &name)
{
    if (m_bindings.contains(name)) {
        return m_bindings[name];
    } else {
        throw std::runtime_error(std::string("Missing binding: ") + name.toStdString());
    }
}
