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
    if (!msg.isEmpty()) {
        emit msgFromWorker(msg);
    }
}

void ValidatingInterceptor::validate(WorkerMsg &msg)
{
    for (auto iter = d->settings.final_by_validator.cbegin(); iter != d->settings.final_by_validator.cend(); ++iter) {
        auto &validator = iter.key();
        for (const auto &field: iter.value()) {
            if (!msg.contains(field)) continue;
            validator.validate(msg[field]);
        }
    }
    for (auto iter = d->settings.final_by_validator_glob.cbegin(); iter != d->settings.final_by_validator_glob.cend(); ++iter) {
        auto validator = iter.key();
        for (auto &msgIter: msg) {
            auto keyJoined = msgIter.key().join(':');
            auto shouldValidate = iter.value().match(keyJoined).hasMatch();
            if (shouldValidate) {
                if (!msg.contains(keyJoined)) continue;
                validator.validate(msg[keyJoined]);
            }
        }
    }
}
