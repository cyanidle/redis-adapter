#include "validatinginterceptor.h"
#include "broker/workers/private/workermsg.h"
#include "validatinginterceptor_settings.h"

using namespace Radapter;

struct Radapter::ValidatingInterceptorPrivate {
    Settings::ValidatingInterceptor settings;
};

ValidatingInterceptor::ValidatingInterceptor(const Settings::ValidatingInterceptor &settings) :
    d(new ValidatingInterceptorPrivate{settings})
{
}

ValidatingInterceptor::~ValidatingInterceptor()
{
    delete d;
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
            validator.validate(msg[field]);
        }
    }
    for (auto iter = d->settings.final_by_validator_glob.cbegin(); iter != d->settings.final_by_validator_glob.cend(); ++iter) {
        auto validator = iter.key();
        for (auto &msgiter: msg) {
            auto keyJoined = msgiter.key().join(':');
            auto shouldValidate = iter.value().match(keyJoined).hasMatch();
            if (shouldValidate) {
                validator.validate(msg[keyJoined]);
            }
        }
    }
}
