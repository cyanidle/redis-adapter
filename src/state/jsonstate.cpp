#include "jsonstate.h"
#include "jsondict/jsondict.h"
#include "settings-parsing/serializablesetting.h"
#include "templates/algorithms.hpp"
#include <QStringBuilder>

struct State::Json::Private {
    Radapter::Worker *parent;
    mutable bool checkPassed{false};
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
    performCheck();
    JsonDict all(structure());
    return QStringLiteral("\nJsonState: ")%this->metaObject()->className()%": Expected --> "%all.print();
}

JsonDict State::Json::send() const
{
    performCheck();
    return serialize();
}

JsonDict State::Json::send(const Serializable::IsFieldCheck &field) const
{
    performCheck();
    return send(findNameOf(field));
}

bool State::Json::updateWith(const Settings::Serializable &setting)
{
    performCheck();
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

void State::Json::performCheck() const
{
    if (d->checkPassed) return;
    for (auto &name: fields()) {
        if (field(name)->isSequence(this) || field(name)->isMapping(this)) {
            if (!d->parent) {
                reWarn() << metaObject()->className() << ": Containers forbidden in JsonState:" << name;
            } else {
                workerWarn(d->parent) << ": Containers forbidden in JsonState:" << name;
            }
        }
    }
    d->checkPassed = true;
}

bool State::Json::updateWith(const JsonDict &data)
{
    performCheck();
    using Radapter::keyVal;
    emit obj->beforeUpdate(data, this);
    bool wasUpdated = false;
    const auto &asMap = QVariantMap(data);
    for (auto [key, val]: keyVal(asMap)) {
        auto found = field(key);
        if (found) {
            wasUpdated |= field(key)->updateWithVariant(this, val); // stays true once set
        } else {
            if (!d->parent) {
                reWarn() << metaObject()->className() << ": Extra property passed:" << key;
            } else {
                workerWarn(d->parent) << ": Extra property passed to state:" << key;
            }
        }
    }
    if (wasUpdated) {
        postUpdate();
        emit obj->wasUpdated(this);
    }
    return wasUpdated;
}

bool State::Json::update(const QVariantMap &data)
{
    return updateWith(data);
}
