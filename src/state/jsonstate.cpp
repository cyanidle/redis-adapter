#include "jsonstate.h"
#include "jsondict/jsondict.h"
#include <QStringBuilder>

void State::Json::addValidatorTo(const QString &field, const QString &validator, ValidateOn role)
{
    if (!fields().contains(field)) {
        reError() << metaObject() << "Could not add validator: ("%validator%") to field: ("%field%')';
        return;
    }
    if (role.testFlag(Update)) {
        m_updateValidators[field] = Validator::Fetched(validator);
    }
    if (role.testFlag(Send)) {
        m_sendValidators[field] = Validator::Fetched(validator);
    }
}

void State::Json::addValidatorTo(const Serializable::IsFieldCheck &field, const QString &validator, ValidateOn role)
{
    addValidatorTo(findNameOf(field), validator, role);
}

QString State::Json::logFields() const
{
    fillCache();
    JsonDict all;
    for (const auto &name: qAsConst(fields()))
    {
        all[m_nestedFieldsCache[name]] = field(name)->typeName(this);
    }
    return QStringLiteral("\nJsonState: ")%this->metaObject()->className()%": Expected --> "%all.print();
}

JsonDict State::Json::send(const Serializable::IsFieldCheck &field) const
{
    auto name = findNameOf(field);
    if (name.isEmpty()) {
        reError() << "#### Attempt to send nonexistent field of JsonState!";
        return {};
    }
    return send(name);
}

JsonDict State::Json::send(const QString &fieldName) const
{
    fillCache();
    JsonDict result;
    if (!fieldName.isEmpty()) {
        auto found = field(fieldName);
        if (!found) {
            QString asPrefix = fieldName%QStringLiteral("__");
            for (auto &name: qAsConst(fields())) {
                if (name.startsWith(asPrefix)) {
                    result[m_nestedFieldsCache[name]] = field(name)->readVariant(this);
                }
            }
            if (result.isEmpty()) {
                reError() << "#### Attempt to send nonexistent field of JsonState:" << fieldName;
            }
            return result;
        }
        auto temp = found->readVariant(this);
        if (m_sendValidators.contains(fieldName)) {
            m_sendValidators[fieldName].validate(temp);
        }
        result.insert(m_nestedFieldsCache[fieldName], temp);
        return result;
    }
    for (const auto &fieldName: fields()) {
        auto found = field(fieldName);
        auto temp = found->readVariant(this);
        if (m_sendValidators.contains(fieldName)) {
            m_sendValidators[fieldName].validate(temp);
        }
        result.insert(m_nestedFieldsCache[fieldName], temp);
    }
    return result;
}

bool State::Json::updateWith(const JsonDict &data)
{
    fillCache();
    bool status = false;
    for (const auto &name: qAsConst(fields()))
    {
        if (data.contains(m_nestedFieldsCache[name])) {
            auto temp = data.value(m_nestedFieldsCache[name]);
            if (m_updateValidators.contains(name)) {
                m_updateValidators[name].validate(temp);
            }
            status |= field(name)->updateWithVariant(this, temp); // stays true once set
        }
    }
    if (status) emit wasUpdated(this);
    return status;
}

void State::Json::fillCache() const
{
    if (cacheFilled) return;
    for (const auto &name: qAsConst(fields())) {
        m_nestedFieldsCache[name] = name.split(QStringLiteral("__"));
    }
    cacheFilled = true;
}

