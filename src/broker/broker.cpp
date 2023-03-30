#include "broker.h"
#include "radapterlogging.h"
#include <QCoreApplication>

using namespace Radapter;

QRecursiveMutex Broker::m_mutex;

Broker::Broker() :
    QObject(),
    m_proxies(),
    m_connected(),
    m_wasMassConnectCalled(false),
    m_debugTable()
{
}

bool Broker::isDebugEnabled(const QString &workerName, QtMsgType type)
{
    return m_debugTable.value(workerName).value(type, true);
}

void Broker::applyWorkerLoggingFilters(const QMap<QString, QMap<QtMsgType, bool>> &table)
{
    for (auto iter{table.begin()}; iter != table.end(); ++iter) {
        m_debugTable[iter.key()] = {iter.value().cbegin(), iter.value().cend()};
    }
}

// Doesnt need mutex, bc is always connected via Qt::QueuedConnection
void Broker::onMsgFromWorker(const Radapter::WorkerMsg &msg)
{
    if (msg.receivers().isEmpty()) {
        brokerWarn() << "Msg with no receivers! Sender:" << msg.sender();
        return;
    }
    if (msg.isDirect() || msg.isBroadcast()) {
        emit broadcastToAll(msg);
    }
}

void Broker::proxyDestroyed(QObject *proxy)
{
    auto casted = qobject_cast<WorkerProxy*>(proxy);
    if (!casted) {
        brokerError() << "Invalid proxy: " << proxy;
        return;
    }
    if (m_proxies.contains(casted->proxyName())) {
        m_proxies.remove(casted->proxyName());
    } else {
        brokerError() << "Proxy already removed: " << proxy;
    }
}

void Broker::registerProxy(WorkerProxy* proxy)
{
    QMutexLocker locker(&m_mutex);
    if (!proxy) {
        throw std::runtime_error("Nullptr passed to broker.registerProxy()!");
    }
    if (proxy->proxyName().isEmpty()) {
        throw std::runtime_error(std::string("Proxy (Worker) name cannot be empty! Source: ") + proxy->parent()->metaObject()->className());
    }
    connect(this, &Broker::broadcastToAll,
            proxy, &WorkerProxy::onMsgFromBroker,
            thread() == proxy->workerThread() ? Qt::DirectConnection : Qt::QueuedConnection);
    connect(proxy, &WorkerProxy::msgToBroker,
            this, &Broker::onMsgFromWorker,
            thread() == proxy->workerThread() ? Qt::DirectConnection : Qt::QueuedConnection);
    connect(proxy->worker(), &Worker::fireEvent,
            this, &Broker::onEvent,
            thread() == proxy->workerThread() ? Qt::DirectConnection : Qt::QueuedConnection);
    connect(this, &Broker::fireEvent,
            proxy->worker(), &Worker::onEvent,
            thread() == proxy->workerThread() ? Qt::DirectConnection : Qt::QueuedConnection);
    connect(proxy, &WorkerProxy::destroyed, this, &Broker::proxyDestroyed,
            thread() == proxy->workerThread() ? Qt::DirectConnection : Qt::QueuedConnection);
    if (m_proxies.contains(proxy->proxyName())) {
        brokerError() << "Broker: Proxy with duplicate name: " << proxy->proxyName();
        throw std::runtime_error(std::string("Broker: Proxy with duplicate name: ") +
                                 std::string(proxy->proxyName().toStdString()));
    } else {
        brokerInfo() << "Broker: Registering proxy with name: " << proxy->proxyName();
        m_proxies.insert(proxy->proxyName(), proxy);
    }
}

void Broker::connectProducersAndConsumers()
{
    QMutexLocker locker(&m_mutex);
    if (m_wasMassConnectCalled) {
        brokerError() << "Broker: connectProducersAndConsumers() was already called!";
        throw std::runtime_error("Broker: connectProducersAndConsumers() was already called!");
    }
    m_wasMassConnectCalled = true;
    for (auto proxyIter = m_proxies.constBegin(); proxyIter != m_proxies.constEnd(); ++proxyIter) {
        for (auto &consumer : proxyIter.value()->consumersNames()) {
            connectTwoProxies(proxyIter.key(), consumer);
        }
        for (auto &producer : proxyIter.value()->producersNames()) {
            connectTwoProxies(producer, proxyIter.key());
        }
    }
}

void Broker::runAll()
{
    QMutexLocker locker(&m_mutex);
    for (auto &proxy : m_proxies) {
        auto worker = proxy->worker();
        if (!worker->wasStarted()) worker->run();
    }
    for (auto &proxy : m_proxies) {
        auto worker = proxy->worker();
        if (!worker->wasStarted()) throw std::runtime_error(std::string("WorkerBase::onRun() not called for: ") +
                                                            worker->metaObject()->className() +
                                                            ": (" + worker->objectName().toStdString() + ")");
    }
}

void Broker::publishEvent(const BrokerEvent &event)
{
    emit fireEvent(event);
}

bool Broker::exists(const QString &workerName) const
{
    QMutexLocker locker(&m_mutex);
    return m_proxies.contains(workerName);
}

Worker* Broker::getWorker(const QString &workerName)
{
    QMutexLocker locker(&m_mutex);
    if (m_proxies.contains(workerName)) {
        return m_proxies[workerName]->worker();
    } else {
        return nullptr;
    }
}

bool Broker::wasStarted()
{
    QMutexLocker locker(&m_mutex);
    return m_wasMassConnectCalled;
}

void Broker::connectTwoProxies(const QString &producer, const QString &consumer)
{
    QMutexLocker locker(&m_mutex);
    if (!m_proxies.value(producer)) {
        brokerWarn() << "Broker: connectProxies: No proxy (producer) with name: " << producer;
        brokerWarn() << "^ Wanted by: " << consumer;
        throw std::runtime_error("Broker: connectProxies(): missing producer --> " + producer.toStdString());
    }
    if (!m_proxies.value(consumer)) {
        brokerWarn() << "Broker: connectProxies: No proxy (consumer) with name: " << consumer;
        brokerWarn() << "^ Wanted by: " << producer;
        throw std::runtime_error("Broker: connectProxies(): missing consumer --> " + consumer.toStdString());
    }
    auto producerPtr = m_proxies.value(producer);
    auto consumerPtr = m_proxies.value(consumer);
    connectTwoProxies(producerPtr, consumerPtr);
}

void Broker::connectTwoProxies(WorkerProxy* producer, WorkerProxy* consumer)
{
    QMutexLocker locker(&m_mutex);
    auto pair = QPair<WorkerProxy*, WorkerProxy*>{producer, consumer};
    if (m_connected.contains(pair)) {
        return;
    }
    brokerInfo() << "\nConnecting:\n == Producer(" << producer->worker()->printSelf()
                 << ") -->\n == Consumer(" << consumer->worker()->printSelf() << ")";
    connect(producer, &WorkerProxy::msgToBroker,
            consumer, &WorkerProxy::onMsgFromBroker,
            consumer->workerThread() == producer->workerThread()
                ? Qt::DirectConnection
                : Qt::QueuedConnection);
    m_connected.append(pair);
    if (!producer->consumers().contains(consumer->worker())) {
        producer->addConsumer(consumer->worker());
    }
    if (!consumer->producers().contains(producer->worker())) {
        consumer->addProducer(producer->worker());
    }
}

void Broker::onEvent(const BrokerEvent &event)
{
    emit fireEvent(event);
}
