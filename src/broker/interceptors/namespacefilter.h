#ifndef RADAPTER_NAMESPACEFILTER_H
#define RADAPTER_NAMESPACEFILTER_H

#include "interceptor.h"

namespace Radapter {

class NamespaceFilter : public Radapter::InterceptorBase
{
    Q_OBJECT
public:
    NamespaceFilter(const QStringList& targetNamespace);
public slots:
    virtual void onMsgFromBroker(const Radapter::WorkerMsg &msg) override;
private:
    QList<QStringList> m_namespaces;
};

} // namespace Radapter

#endif // RADAPTER_NAMESPACEFILTER_H
