#ifndef RADAPTER_NAMESPACEFILTER_H
#define RADAPTER_NAMESPACEFILTER_H

#include "broker/interceptor/interceptor.h"

namespace Radapter {

class NamespaceFilter : public Radapter::Interceptor
{
    Q_OBJECT
public:
    NamespaceFilter(const QString& targetNamespace);
public slots:
    virtual void onMsgFromWorker(Radapter::WorkerMsg &msg) override;
private:
    QStringList m_namespace;
};

} // namespace Radapter

#endif // RADAPTER_NAMESPACEFILTER_H
