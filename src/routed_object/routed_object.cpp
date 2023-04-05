#include "routed_object.h"
#include "routed_object/routesprovider.h"

RoutedObject::RoutedObject(const QString &bindingName, const JsonRoute::KeysFilter &keysFilter) :
    RoutedObject(JsonRoutesProvider::instance()->getBinding(bindingName), keysFilter)
{}

RoutedObject::RoutedObject(const JsonRoute &binding, const JsonRoute::KeysFilter &keysFilter) :
    m_binding(binding.optimise(keysFilter))
{
}

void RoutedObject::routedUpdate(const JsonDict &data)
{
    checkIfOk();
    auto result = m_binding.receive(data);
    if (!result.isEmpty()) update(result.data());
}

const QStringList RoutedObject::mappedFields(const QString &separator) const
{
    return send().flatten(separator).keys();
}

const QStringList RoutedObject::mappedFieldsExclude(const QStringList &exclude, const QString &separator) const
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

const QString RoutedObject::mappedField(const QString &field, const QString &separator) const
{
    return send(field).keys(separator).first();
}

JsonDict RoutedObject::send(const QString &fieldName) const
{
    checkIfOk();
    if (fieldName.isEmpty()) {
        return m_binding.send(this->serialize());
    } else {
        auto foundField = field(fieldName);
        if (!foundField) throw std::runtime_error("Field: " + fieldName.toStdString() + ": does not exist!");
        return m_binding.send({{fieldName, foundField->readVariant(this)}});
    };
}

JsonDict RoutedObject::sendGlob(const QString &glob) const
{
    auto globre = QRegularExpression(QRegularExpression::wildcardToRegularExpression(glob));
    if (!globre.isValid()) {
        throw std::invalid_argument("Glob syntax error!");
    }
    JsonDict result;
    for (auto &field : fields()) {
        if (globre.globalMatch(field).isValid()) {
            result.merge(send(field));
        }
    }
    return result;
}

QString RoutedObject::print() const
{
    return JsonDict{serialize()}.printDebug().replace("Json", metaObject()->className());
}

bool RoutedObject::wasUpdated(const QString &fieldName) const
{
    return m_wereUpdated.contains(fieldName);
}

bool RoutedObject::wasUpdatedGlob(const QString &glob) const
{
    auto globre = QRegularExpression(QRegularExpression::wildcardToRegularExpression(glob));
    if (!globre.isValid()) {
        throw std::invalid_argument("Glob syntax error!");
    }
    for (auto &field : fields()) {
        if (globre.globalMatch(field).isValid() && wasUpdated(field)) {
            return true;
        }
    }
    return false;
}

bool RoutedObject::isIgnored(const QString &fieldName) const
{
    return m_binding.ignoredFields().contains(fieldName);
}

void RoutedObject::checkIfOk() const
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

bool RoutedObject::update(const QVariantMap &src)
{
    m_wereUpdated.clear();
    for (const auto &fieldName : fields()) {
        if(field(fieldName)->updateWithVariant(this, src.value(fieldName))) {
            m_wereUpdated.append(fieldName);
        }
    }
    return m_wereUpdated.size();
}
