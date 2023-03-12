#include "namespacefilter.h"

namespace Radapter {

NamespaceFilter::NamespaceFilter(const QStringList& targetNamespace) :
    m_namespaces()
{
    for (auto &rawNamespace : targetNamespace) {
        m_namespaces.append(rawNamespace.split(":"));
    }
}

void NamespaceFilter::onMsgFromBroker(const Radapter::WorkerMsg &msg)
{
    auto copy = msg;
    copy.clearJson();
    bool hasAny = false;
    for (auto &currNamespace : m_namespaces) {
        hasAny = true;
        if (msg.contains(currNamespace)) {
            copy[currNamespace] = msg[currNamespace];
        }
    }
    if (hasAny) {
        emit msgToWorker(copy);
    }
}

} // namespace Radapter
