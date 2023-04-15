#include "broker.h"
#include "broker/brokersettings.h"
#include "broker/interceptor/interceptor.h"
#include "broker/workers/private/workermsg.h"
#include "radapterlogging.h"
#include <QMutex>
#include <QMutexLocker>
#include "templates/algorithms.hpp"
#include "workers/worker.h"
#include "workers/private/workerproxy.h"
#include <QCoreApplication>
using namespace Radapter;

struct WorkerConnection {
    Worker *producer;
    WorkerProxy *producerProxy;
    Worker *consumer;
    void kill() {
        producerProxy->deleteLater();
    }
    bool matches(const Worker *producer, const Worker *consumer) const {
        return this->producer==producer && this->consumer==consumer;
    }
};

struct Radapter::BrokerPrivate {
    QMap<QString, Worker*> workers;
    QMap<QString, Interceptor*> interceptors;
    QList<WorkerConnection> connections;
    Settings::Broker settings;
    QRecursiveMutex mutex;
    bool wereConnected(const Worker *producer, const Worker *consumer) const {
        for (const auto &conn: connections) {
            if (conn.matches(producer, consumer)) {
                return true;
            }
        }
        return false;
    }
};

Broker::Broker() :
    QObject(),
    d(new BrokerPrivate)
{
}

void Broker::onMsgFromWorker(const Radapter::WorkerMsg &msg)
{
    if (msg.isBroadcast()) {
        emit broadcastToAll(msg);
        return;
    }
    if (!msg.sender()) {
        throw std::runtime_error("Cannot have msg without sender!");
    }
    auto copy = msg;
    copy.receivers().subtract(msg.sender()->consumers()); // consumers already received the msg
    emit broadcastToAll(copy);
    if (msg.receivers().isEmpty()) {
        if (d->settings.warn_no_receivers) {
            brokerWarn() << "Msg with no receivers! Sender:" << msg.sender();
        }
        return;
    }
}

void Broker::proxyDestroyed(QObject *proxy)
{
    auto casted = qobject_cast<WorkerProxy*>(proxy);
    if (!casted) {
        brokerError() << "Invalid proxy: " << proxy;
        return;
    }
    d->connections.removeIf([&](const WorkerConnection &conn) {
        return conn.producerProxy == casted;
    });
}

void Broker::registerWorker(Worker* worker)
{
    QMutexLocker locker(&d->mutex);
    if (!worker) {
        throw std::runtime_error("Nullptr passed to broker.registerProxy()!");
    }
    if (worker->workerName().isEmpty()) {
        throw std::runtime_error(std::string("Worker name cannot be empty! Source: ") + worker->parent()->metaObject()->className());
    }
    connect(this, &Broker::broadcastToAll,
            worker, &Worker::onMsgFromBroker,
            thread() == worker->workerThread() ? Qt::DirectConnection : Qt::QueuedConnection);
    connect(worker, &Worker::sendMsg, this, &Broker::onMsgFromWorker,
            thread() == worker->workerThread() ? Qt::DirectConnection : Qt::QueuedConnection);
    brokerInfo() << "Registering worker:" << worker->printSelf();
    d->workers.insert(worker->workerName(), worker);
}

void Broker::runAll()
{
    QMutexLocker locker(&d->mutex);
    for (auto worker : d->workers) {
        if (!worker->wasStarted()) worker->run();
    }
    for (auto &worker : d->workers) {
        if (!worker->wasStarted()) throw std::runtime_error(std::string("WorkerBase::onRun() not called for: ") +
                                                            worker->metaObject()->className() +
                                                            ": (" + worker->objectName().toStdString() + ")");
    }
}

Broker::~Broker()
{
    delete d;
}

void Broker::applySettings(const Settings::Broker &newSettings)
{
    QMutexLocker locker(&d->mutex);
    d->settings = newSettings;
}

bool Broker::exists(const QString &workerName) const
{
    QMutexLocker locker(&d->mutex);
    return d->workers.contains(workerName);
}

Worker* Broker::getWorker(const QString &workerName)
{
    QMutexLocker locker(&d->mutex);
    if (d->workers.contains(workerName)) {
        return d->workers[workerName];
    } else {
        return nullptr;
    }
}

void Broker::registerInterceptor(const QString &name, Interceptor *interceptor)
{
    interceptor->setObjectName(name);
    brokerInfo() << "Registering interceptor:" << name
                 << QStringLiteral("(%1)").arg(interceptor->metaObject()->className());
    d->interceptors.insert(name, interceptor);
}

Interceptor *Broker::getInterceptor(const QString &name) const
{
    return d->interceptors.value(name);
}

void Broker::connectTwo(const QString &producer, const QString &consumer, const QStringList &interceptorNames)
{
    QMutexLocker locker(&d->mutex);
    if (!d->workers.value(producer)) {
        brokerWarn() << "Broker: connectTwo: No producer with name: " << producer;
        brokerWarn() << "^ Wanted by: " << consumer;
        throw std::runtime_error("Broker: connectTwo(): missing producer --> " + producer.toStdString());
    }
    if (!d->workers.value(consumer)) {
        brokerWarn() << "Broker: connectTwo: No consumer with name: " << consumer;
        brokerWarn() << "^ Wanted by: " << producer;
        throw std::runtime_error("Broker: connectTwo(): missing consumer --> " + consumer.toStdString());
    }
    auto producerPtr = d->workers.value(producer);
    auto consumerPtr = d->workers.value(consumer);
    QList<Interceptor*> interceptors;
    for (const auto &name : interceptorNames) {
        auto current = getInterceptor(name);
        if (!current) {
            brokerWarn() << "Broker: connectTwo: No interceptor with name: " << name;
            brokerWarn() << "^ Wanted by: " << producer << "-->" << consumer;
            throw std::runtime_error("Broker: connectTwo(): missing interceptor --> " + name.toStdString());
        }
        interceptors.append(current);
    }
    connectProxyToWorker(producerPtr->createPipe(interceptors), consumerPtr);
}

void Broker::connectTwo(Worker *producer, Worker *consumer, const QList<Interceptor *> interceptors)
{
    connectProxyToWorker(producer->createPipe(interceptors), consumer);
}

bool Broker::areConnected(Worker *producer, Worker *consumer)
{
    return d->wereConnected(producer, consumer);
}

void Broker::disconnect(Worker *producer, Worker *consumer)
{
    for (auto &conn: d->connections) {
        if (conn.producer == producer && conn.consumer == consumer) {
            conn.kill();
            return;
        }
    }
    throw std::runtime_error("Cannot disconnect unconnected workers!");
}

void Broker::connectProxyToWorker(WorkerProxy* producerProxy, Worker *consumer)
{
    QMutexLocker locker(&d->mutex);
    if (producerProxy->worker() == consumer && !d->settings.allow_self_connect) {
        throw std::runtime_error("Attempt to connect worker to itself! Can be enabled by broker option: 'allow_self_connect'");
    }
    const auto conn = WorkerConnection{producerProxy->worker(), producerProxy, consumer};
    if (d->wereConnected(conn.producer, conn.consumer)) {
        throw std::runtime_error("Duplicate connection between '"
                                 + conn.producer->printSelf().toStdString()
                                 + "' and '"
                                 + conn.consumer->printSelf().toStdString() + "'");
    }
    QStringList interceptors;
    for (auto inter: conn.producer->pipe(producerProxy)) {
        interceptors.append(inter->objectName());
    }
    auto interceptorsMsg = '|' + interceptors.join("| --> |") + '|';
    brokerInfo() << "\nConnecting:\n == Producer(" << conn.producer->printSelf()
                 << ") -->\n"
                 << "== Pipe(" << interceptorsMsg
                 << ") -->\n"
                 << "== Consumer(" << conn.consumer->printSelf() << ")";
    connect(producerProxy, &WorkerProxy::msgToConsumers,
            consumer, &Worker::onMsgFromBroker,
            consumer->workerThread() == producerProxy->workerThread()
                ? Qt::DirectConnection
                : Qt::QueuedConnection);
    connect(producerProxy, &WorkerProxy::msgToBroker,
            this, &Broker::onMsgFromWorker,
            producerProxy->workerThread() == thread()
                ? Qt::DirectConnection
                : Qt::QueuedConnection);
    d->connections.append(conn);
    if (!producerProxy->consumers().contains(consumer)) {
        producerProxy->addConsumer(consumer);
    }
    if (!consumer->producers().contains(producerProxy->worker())) {
        consumer->addProducer(producerProxy->worker());
    }
}

Broker *Radapter::Broker::instance() {
    static Broker* broker {new Broker()};
    return broker;
}

QSet<Worker *> Broker::getAll(const QMetaObject *mobj)
{
    QSet<Worker *> result;
    for (auto worker: d->workers) {
        if (worker->metaObject()->inherits(mobj)){
            result.insert(worker);
        }
    }
    return result;
}
