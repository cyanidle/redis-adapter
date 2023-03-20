#include "routed.h"
#include "bindings/routesprovider.h"

Routed::Routed(const QString &bindingName, const JsonRoute::KeysFilter &keysFilter) :
    Routed(JsonRoutesProvider::instance()->getBinding(bindingName), keysFilter)
{}

Routed::Routed(const JsonRoute &binding, const JsonRoute::KeysFilter &keysFilter) :
    m_binding(binding.optimise(keysFilter))
{
}

RoutedGadget::RoutedGadget(const JsonRoute &binding, const JsonRoute::KeysFilter &keysFilter) :
    GadgetMixin(binding, keysFilter)
{

}

RoutedGadget::RoutedGadget(const QString &bindingName, const JsonRoute::KeysFilter &keysFilter) :
    GadgetMixin(JsonRoutesProvider::instance()->getBinding(bindingName), keysFilter)
{

}

RoutedQObject::RoutedQObject(const QString &bindingName, const JsonRoute::KeysFilter &keysFilter, QObject *parent) :
    RoutedQObject(JsonRoutesProvider::instance()->getBinding(bindingName), keysFilter, parent)
{

}

RoutedQObject::RoutedQObject(const JsonRoute &binding, const JsonRoute::KeysFilter &keysFilter, QObject *parent) :
    QObjectMixin(parent, binding, keysFilter)
{

}

void RoutedQObject::routedUpdate(const JsonDict &data)
{
    Routed::routedUpdate(data);
}

void Routed::routedUpdate(const JsonDict &data)
{
    checkIfOk();
    auto result = m_binding.receive(data);
    if (!result.isEmpty()) update(result.data());
}

const QStringList Routed::mappedFields(const QString &separator) const
{
    return send().flatten(separator).keys();
}

const QStringList Routed::mappedFieldsExclude(const QStringList &exclude, const QString &separator) const
{
    if (exclude.isEmpty()) {
        return send().flatten(separator).keys();
    } else {
        QStringList result;
        for (auto &field : fields()) {
            if (!exclude.contains(field)) {
                result.append(send(field).keys(separator).first());
            }
        }
        return result;
    }
}

const QString Routed::mappedField(const QString &field, const QString &separator) const
{
    return send(field).keys(separator).first();
}

JsonDict Routed::send(const QString &fieldName) const
{
    checkIfOk();
    if (fieldName.isEmpty()) {
        return m_binding.send(this->serialize());
    } else {
        auto foundField = field(fieldName);
        if (!foundField) throw std::runtime_error("Field: " + fieldName.toStdString() + ": does not exist!");
        return m_binding.send({{fieldName, foundField->readVariant()}});
    };
}

JsonDict Routed::sendGlob(const QString &glob) const
{
    auto globre = QRegExp(glob);
    globre.setPatternSyntax(QRegExp::WildcardUnix);
    if (!globre.isValid()) {
        throw std::invalid_argument("Glob syntax error!");
    }
    JsonDict result;
    for (auto &field : fields()) {
        if (globre.exactMatch(field)) {
            result.merge(send(field));
        }
    }
    return result;
}

bool Routed::wasUpdated(const QString &fieldName) const
{
    return m_wereUpdated.contains(fieldName);
}

bool Routed::wasUpdatedGlob(const QString &glob) const
{
    auto globre = QRegExp(glob);
    globre.setPatternSyntax(QRegExp::WildcardUnix);
    if (!globre.isValid()) {
        throw std::invalid_argument("Glob syntax error!");
    }
    for (auto &field : fields()) {
        if (globre.exactMatch(field) && wasUpdated(field)) {
            return true;
        }
    }
    return false;
}

bool Routed::isIgnored(const QString &fieldName) const
{
    return m_binding.ignoredFields().contains(fieldName);
}

void Routed::checkIfOk() const
{
    if (m_checkPassed) {
        return;
    }
    for (auto &field : fields()) {
        try {
            m_binding.requireValueName(field);
        } catch (std::exception &e){
            brokerError() << "Required: " << fields();
            throw std::runtime_error(e.what());
        }
    }
    m_checkPassed = true;
}

bool Routed::update(const QVariantMap &src)
{
    m_wereUpdated.clear();
    for (const auto &fieldName : fields()) {
        if(field(fieldName)->updateWithVariant(src.value(fieldName))) {
            m_wereUpdated.append(fieldName);
        }
    }
}
