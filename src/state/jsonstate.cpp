#include "jsonstate.h"
#include "jsondict/jsondict.h"
#include <QStringBuilder>

QString State::Json::logFields() const
{
    JsonDict all;
    for (const auto &name: qAsConst(fields()))
    {
        all[name] = field(name)->typeName(this);
    }
    return QStringLiteral("\nJsonState: ")%this->metaObject()->className()%": Expected --> "%all.print();
}

JsonDict State::Json::send() const
{
    return serialize();
}

JsonDict State::Json::send(const Serializable::IsFieldCheck &field) const
{
    return send(findNameOf(field));
}

JsonDict State::Json::send(const QString &fieldName) const
{
    JsonDict result;
    if (!fieldName.isEmpty()) {
        auto found = field(fieldName);
        if (!found) {
            reError() << "#### Attempt to send nonexistent field of JsonState:" << fieldName;
            return result;
        }
        result.insert(fieldName, found->readVariant(this));
        return result;
    } else {
        reError() << "#### Attempt to send field with empty name!" << fieldName;
        return {};
    }
}

bool State::Json::updateWith(const JsonDict &data)
{
    emit beforeUpdateSig(data, this);
    bool status = false;
    for (const auto &name: qAsConst(fields()))
    {
        if (data.contains(name)) {
            auto temp = data.value(name);
            status |= field(name)->updateWithVariant(this, temp); // stays true once set
        }
    }
    if (status) emit wasUpdated(this);
    return status;
}

bool State::Json::update(const QVariantMap &data)
{
    return updateWith(data);
}
