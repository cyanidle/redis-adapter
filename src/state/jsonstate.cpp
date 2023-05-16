#include "jsonstate.h"
#include "jsondict/jsondict.h"
#include "settings-parsing/serializablesetting.h"
#include "templates/algorithms.hpp"
#include <QStringBuilder>

struct State::Json::Private {
    Radapter::Worker *parent;
};

State::Json::Json(Radapter::Worker *parent) :
    obj(new State::Private::JsonStateQObject),
    d(new Private{parent})
{

}

State::Json::~Json()
{
    delete d;
    delete obj;
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
    using Radapter::keyVal;
    emit obj->beforeUpdate(data, this);
    bool status = false;
    const auto &asMap = QVariantMap(data);
    for (auto [key, val]: keyVal(asMap)) {
        auto found = field(key);
        if (found) {
            status |= field(key)->updateWithVariant(this, val); // stays true once set
        } else {
            if (!d->parent) {
                reWarn() << metaObject()->className() << ": Extra property passed:" << key;
            } else {
                workerWarn(d->parent) << ": Extra property passed to state:" << key;
            }
        }
    }
    if (status) {
        postUpdate();
        emit obj->wasUpdated(this);
    }
    return status;
}

bool State::Json::update(const QVariantMap &data)
{
    return updateWith(data);
}
