#include "validatinginterceptor.h"
#include "broker/workers/private/workermsg.h"
#include "settings/validatinginterceptorsettings.h"
#include "templates/algorithms.hpp"

using namespace Radapter;

struct Radapter::ValidatingInterceptorPrivate {
    Settings::ValidatingInterceptor settings;
};

ValidatingInterceptor::ValidatingInterceptor(const Settings::ValidatingInterceptor &settings) :
    d(new ValidatingInterceptorPrivate{settings})
{
    d->settings.init();
}

ValidatingInterceptor::~ValidatingInterceptor()
{
    delete d;
}

Radapter::Interceptor *ValidatingInterceptor::newCopy() const
{
    return new ValidatingInterceptor(d->settings);
}

void ValidatingInterceptor::onMsgFromWorker(WorkerMsg &msg)
{
    validate(msg);
    msg.json() = msg.sanitized();
    if (!msg.isEmpty()) {
        emit msgFromWorker(msg);
    }
}

bool listStartsWith(const QStringList &who, const QStringList &with)
{
    for (auto [f, s]: zip(who, with)) {
        if (f != s) return false;
    }
    return true;
}

void ValidatingInterceptor::checkKeyVal(const QStringList &key, QVariant& val)
{
    auto joined = key.join(':');
    for (auto iter = d->settings.final_by_validator.cbegin(); iter != d->settings.final_by_validator.cend(); ++iter) {
        auto &validator = iter.key();
        for (const auto &[field, split]: iter.value()) {
            auto hit = field == joined;
            hit |= listStartsWith(split, key);
            if (hit ^ d->settings.inverse) {
                if (!validator.validate(val)) {
                    val.clear();
                }
            }
        }
    }
    for (auto iter = d->settings.final_by_validator_glob.cbegin(); iter != d->settings.final_by_validator_glob.cend(); ++iter) {
        auto validator = iter.key();
        auto globHit = iter.value().match(joined).hasMatch();
        if (globHit ^ d->settings.inverse) {
            if (!validator.validate(val)) {
                val.clear();
            }
        }
    }
}

void ValidatingInterceptor::validate(WorkerMsg &msg)
{
    for (auto &[key, val]: msg.json()) {
        checkKeyVal(key, val);
    }
}
