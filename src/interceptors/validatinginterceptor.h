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
    Interceptor *newCopy() const override;
public slots:
    virtual void onMsgFromWorker(Radapter::WorkerMsg &msg) override;
private:
    void validate(WorkerMsg &msg);
    void checkKeyVal(const QStringList &key, QVariant &val);

    ValidatingInterceptorPrivate *d;
};

} //namespace Radapter

#endif //VALIDATINGINTERCEPTOR_H
