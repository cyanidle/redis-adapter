#include "validatinginterceptor.h"
#include "broker/workers/private/workermsg.h"
#include "validatinginterceptor_settings.h"

using namespace Radapter;

struct Radapter::ValidatingInterceptorPrivate {
    Settings::ValidatingInterceptor settings;
    JsonDict validatorsState;
};

ValidatingInterceptor::ValidatingInterceptor(const Settings::ValidatingInterceptor &settings) :
    d(new ValidatingInterceptorPrivate{settings, {}})
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
    emit msgFromWorker(msg);
}

void ValidatingInterceptor::validate(WorkerMsg &msg)
{
    for (auto iter = d->settings.final_by_validator.cbegin(); iter != d->settings.final_by_validator.cend(); ++iter) {
        auto &validator = iter.key();
        for (const auto &field: iter.value()) {
            auto &state = d->validatorsState[field];
            validator.validate(msg[field], state);
        }
    }
    for (auto iter = d->settings.final_by_validator_glob.cbegin(); iter != d->settings.final_by_validator_glob.cend(); ++iter) {
        auto validator = iter.key();
        for (auto &msgIter: msg) {
            auto keyJoined = msgIter.key().join(':');
            auto shouldValidate = iter.value().match(keyJoined).hasMatch();
            if (shouldValidate) {
                auto &state = d->validatorsState[keyJoined];
                validator.validate(msg[keyJoined], state);
            }
        }
    }
}
