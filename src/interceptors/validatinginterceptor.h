#ifndef VALIDATINGINTERCEPTOR_H
#define VALIDATINGINTERCEPTOR_H

#include "broker/interceptor/interceptor.h"
#include <QSharedPointer>

namespace Settings {
struct ValidatingInterceptor;
}

namespace Radapter {
struct ValidatingInterceptorPrivate;
class ValidatingInterceptor : public Interceptor {
    Q_OBJECT
public:
    ValidatingInterceptor(const Settings::ValidatingInterceptor& settings);
    ~ValidatingInterceptor();
public slots:
    virtual void onMsgFromWorker(Radapter::WorkerMsg &msg) override;
private:
    void validate(WorkerMsg &msg);

    const ValidatingInterceptorPrivate *d;
};

} //namespace Radapter

#endif //VALIDATINGINTERCEPTOR_H
