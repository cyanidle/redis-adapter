#ifndef RADAPTER_NAMESPACEUNWRAPPER_H
#define RADAPTER_NAMESPACEUNWRAPPER_H

#include "broker/interceptor/interceptor.h"
namespace Settings {
struct NamespaceUnwrapper;
}
namespace Radapter {

class NamespaceUnwrapper : public Radapter::Interceptor
{
    Q_OBJECT
    struct Private;
public:
    NamespaceUnwrapper(const Settings::NamespaceUnwrapper &settings);
    Interceptor *newCopy() const override;
    ~NamespaceUnwrapper() override;
public slots:
    virtual void onMsgFromWorker(Radapter::WorkerMsg &msg) override;
private:
    Private *d;
};

} // namespace Radapter

#endif // RADAPTER_NAMESPACEUNWRAPPER_H
