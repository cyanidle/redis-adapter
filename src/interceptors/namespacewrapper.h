#ifndef RADAPTER_NAMESPACEWRAPPER_H
#define RADAPTER_NAMESPACEWRAPPER_H

#include "broker/interceptor/interceptor.h"
namespace Settings {
struct NamespaceWrapper;
}
namespace Radapter {

class NamespaceWrapper : public Radapter::Interceptor
{
    Q_OBJECT
    struct Private;
public:
    NamespaceWrapper(const Settings::NamespaceWrapper &settings);
    Interceptor *newCopy() const override;
    ~NamespaceWrapper() override;
public slots:
    virtual void onMsgFromWorker(Radapter::WorkerMsg &msg) override;
private:
    Private *d;
};

} // namespace Radapter

#endif // RADAPTER_NAMESPACEWRAPPER_H
