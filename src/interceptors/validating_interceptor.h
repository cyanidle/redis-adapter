#ifndef VALIDATING_INTERCEPTOR_H
#define VALIDATING_INTERCEPTOR_H

#include "broker/interceptor/interceptor.h"
#include <QSharedPointer>

namespace Settings {
struct ValidatingInterceptor;
}

namespace Radapter {

class ValidatingInterceptor : public Interceptor {
    Q_OBJECT
public:
    ValidatingInterceptor(const Settings::ValidatingInterceptor& settings);
public slots:
    virtual void onMsgFromWorker(const Radapter::WorkerMsg &msg) override;
    virtual void onMsgFromBroker(const Radapter::WorkerMsg &msg) override;
private:
    void validate(WorkerMsg &msg);

    QSharedPointer<Settings::ValidatingInterceptor> m_settings;
};

} //namespace Radapter

#endif //VALIDATING_INTERCEPTOR_H
