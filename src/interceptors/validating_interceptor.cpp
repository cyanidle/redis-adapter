#include "validating_interceptor.h"
#include "broker/workers/private/workermsg.h"
#include "validating_interceptor_settings.h"

using namespace Radapter;

ValidatingInterceptor::ValidatingInterceptor(const Settings::ValidatingInterceptor &settings) :
    m_settings(m_settings.create(settings))
{
}

void ValidatingInterceptor::onMsgFromWorker(const WorkerMsg &msg)
{
    auto copy = msg;
    validate(copy);
    emit msgToBroker(copy);
}

void ValidatingInterceptor::onMsgFromBroker(const WorkerMsg &msg)
{
    if (m_settings->validate_incoming) {
        auto copy = msg;
        validate(copy);
        emit msgToWorker(copy);
    } else {
        emit msgToWorker(msg);
    }
}

void ValidatingInterceptor::validate(WorkerMsg &msg)
{
    auto &json = msg.json();
    for (auto iter = m_settings->final_by_validator.cbegin();
            iter != m_settings->final_by_validator.cend();
            ++iter) {
        auto &validator = iter.key();
        for (const auto &field: iter.value()) {
            if (!validator->validate(json[field])) {
                json[field].clear();
            }
        }
    }
}
