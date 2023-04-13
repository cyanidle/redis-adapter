#include "namespacefilter.h"
#include "broker/workers/private/workermsg.h"

namespace Radapter {

NamespaceFilter::NamespaceFilter(const QString& targetNamespace) :
    m_namespace(targetNamespace.split(':'))
{
}

void NamespaceFilter::onMsgFromWorker(WorkerMsg &msg)
{
    auto copy = msg;
    copy.clearJson();
    if (msg.contains(m_namespace)) {
        copy[m_namespace] = msg[m_namespace];
        emit msgFromWorker(copy);
    }
}

} // namespace Radapter
