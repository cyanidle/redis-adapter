#include "validatinginterceptor.h"
#include "broker/workers/private/workermsg.h"
#include "settings/validatinginterceptorsettings.h"

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

void ValidatingInterceptor::checkKeyVal(const QStringList &key, QVariant& val)
{
    auto joined = key.join(':');
    for (auto iter = d->settings.final_by_validator.cbegin(); iter != d->settings.final_by_validator.cend(); ++iter) {
        auto &validator = iter.key();
        for (const auto &field: iter.value()) {
            if (field == joined ^ d->settings.inverse) {
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
