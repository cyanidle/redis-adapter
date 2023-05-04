#include "jsonstate.h"
#include "jsondict/jsondict.h"
#include "settings-parsing/serializablesetting.h"
#include <QStringBuilder>

State::Json::Json() :
    d(new Private::JsonStateQObject)
{
}

State::Json::~Json()
{
    delete d;
}

QString State::Json::logInfo() const
{
    JsonDict all(structure());
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

bool State::Json::updateWith(const Settings::Serializable &setting)
{
    return update(setting.serialize());
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
        reError() << "#### Attempt to send field with empty name!";
        return {};
    }
}

bool State::Json::updateWith(const JsonDict &data)
{
    emit d->beforeUpdate(data, this);
    bool status = false;
    for (const auto &name: qAsConst(fields()))
    {
        if (data.contains(name)) {
            auto temp = data.value(name);
            status |= field(name)->updateWithVariant(this, temp); // stays true once set
        }
    }
    if (status) {
        postUpdate();
        emit d->wasUpdated(this);
    }
    return status;
}

bool State::Json::update(const QVariantMap &data)
{
    return updateWith(data);
}
