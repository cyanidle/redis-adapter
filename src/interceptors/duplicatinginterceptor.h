#ifndef RADAPTER_DUPLICATINGINTERCEPTOR_H
#define RADAPTER_DUPLICATINGINTERCEPTOR_H

#include "broker/interceptor/interceptor.h"
#include <QObject>
#include <QSharedPointer>

namespace Settings {
struct DuplicatingInterceptor;
}
namespace Radapter {

class DuplicatingInterceptor : public Radapter::Interceptor
{
    Q_OBJECT
public:
    DuplicatingInterceptor(const Settings::DuplicatingInterceptor &settings);
    Interceptor *newCopy() const override;
public slots:
    virtual void onMsgFromWorker(Radapter::WorkerMsg &msg) override;
private:
    QSharedPointer<Settings::DuplicatingInterceptor> m_settings;
};

} // namespace Radapter

#endif // RADAPTER_DUPLICATINGINTERCEPTOR_H
