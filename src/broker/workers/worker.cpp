#include "worker.h"
#include "broker/workers/private/pipestart.h"
#include "radapterlogging.h"
#include "broker/broker.h"
#include <QThread>
#include <QMutex>
#include "broker/interceptor/interceptor.h"
#include "workersettings.h"
#include "radapterlogging.h"
#include "private/workerproxy.h"
#include "routed_object/routed_object.h"

using namespace Radapter;

struct WorkerPipe {
    PipeStart *start;
    QList<Interceptor*> interceptors;
    WorkerProxy* proxy;
};

struct Radapter::WorkerPrivate {
    Settings::Worker config;
    QThread *thread;
    QSet<Worker*> consumers;
    WorkerMsg baseMsg;
    QSet<Worker*> producers;
    QHash<WorkerProxy*, WorkerPipe> pipes;
    std::atomic<bool> wasRun;
};

Q_GLOBAL_STATIC(QRecursiveMutex, staticMutex);
Q_GLOBAL_STATIC(QSet<Interceptor*>, staticUsedInterceptors);

bool isLogAllowed(QtMsgType base, QtMsgType target)
{
    switch(base) {
    case QtDebugMsg: return true;
    case QtInfoMsg: return target != QtDebugMsg;
    case QtWarningMsg: return target != QtInfoMsg && target != QtDebugMsg;
    case QtFatalMsg: return target != QtInfoMsg && target != QtDebugMsg && target != QtWarningMsg;
    case QtCriticalMsg: return target != QtInfoMsg && target != QtDebugMsg && target != QtFatalMsg && target != QtWarningMsg;
    default: return true;
    }
}

Worker::Worker(const Settings::Worker &settings, QThread *thread) :
    QObject(),
    d(new WorkerPrivate)
{
    d->thread = thread;
    d->wasRun = false;
    d->baseMsg = {this};
    d->config = settings;
    setObjectName(settings.name);
    if (isPrintMsgsEnabled()) {
        brokerWarn()<< "=== Worker (" << workerName() << "): Running in Debug Mode! ===";
    }
    connect(this, &Worker::sendMsg, &Worker::onSendMsgPriv);
    connect(this, &Worker::sendBasic, &Worker::onSendBasic);
    connect(this, &Worker::sendRouted, &Worker::onSendRouted);
    connect(thread, &QThread::started, this, &Worker::onRun);
    connect(thread, &QThread::destroyed, this, &Worker::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

bool Worker::isPrintMsgsEnabled() const
{
    return d->config.print_msgs;
}

bool Worker::printEnabled(QtMsgType type) const
{
    return isLogAllowed(d->config.log_level.value, type);
}

Broker *Worker::broker() const
{
    return Broker::instance();
}

bool Worker::wasStarted() const
{
    return d->wasRun;
}

bool Worker::is(const QMetaObject *mobj) const
{
    return metaObject()->inherits(mobj);
}

QString Worker::printSelf() const
{
    return QStringLiteral("%2 (%1)").arg(metaObject()->className(), workerName());
}

Worker::~Worker()
{
    delete d;
}

void Worker::run()
{
    QMutexLocker locker(staticMutex);
    if (d->wasRun) throw std::runtime_error("WorkerBase::onRun() called multiple times for: " + printSelf().toStdString());
    moveToThread(workerThread());
    workerThread()->start();
    d->wasRun = true;
}

void Worker::onRun()
{
    workerInfo(this) << "started!";
}

void Worker::addConsumers(const QStringList &consumers)
{
    QMutexLocker locker(staticMutex);
    for (auto &name : consumers) {
        if (wasStarted()) {
            auto worker = broker()->getWorker(name);
            if (!worker) throw std::runtime_error("Nonexistent worker: " + name.toStdString());
            addConsumer(worker);
        }
    }
}

void Worker::addProducers(const QStringList &producers)
{
    QMutexLocker locker(staticMutex);
    for (auto &name : producers) {
        if (wasStarted()) {
            auto worker = broker()->getWorker(name);
            if (!worker) throw std::runtime_error("Nonexistent worker: " + name.toStdString());
            addProducer(worker);
        }
    }
}

void Worker::addConsumers(const WorkerSet &consumers)
{
    QMutexLocker locker(staticMutex);
    for (auto &worker : consumers) {
        addConsumer(worker);
    }
}

void Worker::addProducers(const WorkerSet &producers)
{
    QMutexLocker locker(staticMutex);
    for (auto &worker : producers) {
        addProducer(worker);
    }
}

void Worker::addConsumer(Worker *consumer, QList<Radapter::Interceptor*> interceptors)
{
    QMutexLocker locker(staticMutex);
    d->consumers.insert(consumer);
    if (!consumer->producers().contains(this)) {
        consumer->addProducer(this);
    }
    connect(consumer, &QObject::destroyed, this, &Worker::onWorkerDestroyed);
    d->baseMsg.m_receivers.insert(consumer);
    if (wasStarted()) {
        broker()->connectTwo(this, consumer, interceptors);
    }
}

void Worker::addProducer(Worker *producer, QList<Radapter::Interceptor*> interceptors)
{
    QMutexLocker locker(staticMutex);
    d->producers.insert(producer);
    if (!producer->consumers().contains(this)) {
        producer->addConsumer(this);
    }
    connect(producer, &QObject::destroyed, this, &Worker::onWorkerDestroyed);
    if (wasStarted()) {
        broker()->connectTwo(producer, this, interceptors);
    }
}

WorkerMsg Worker::prepareMsg(const JsonDict &msg) const
{
    auto wrapped = d->baseMsg;
    wrapped.updateId();
    wrapped.setJson(msg);
    return wrapped;
}

WorkerMsg Worker::prepareMsg(JsonDict &&msg) const
{
    auto wrapped = d->baseMsg;
    wrapped.updateId();
    wrapped.setJson(std::move(msg));
    return wrapped;
}

WorkerMsg Worker::prepareReply(const WorkerMsg &msg, Reply *reply) const
{
    if (!msg.sender()) return msg;
    auto rep = msg;
    rep.receivers() = {rep.sender()};
    rep.m_sender = {const_cast<Worker*>(this)};
    rep.setReply(reply);
    return rep;
}

WorkerMsg Worker::prepareCommand(Command *command) const
{
    auto wrapped = d->baseMsg;
    wrapped.setCommand(command);
    wrapped.updateId();
    return wrapped;
}

void Worker::onReply(const Radapter::WorkerMsg &msg)
{
    workerError(this) << ": received Reply from: " <<
        msg.sender()->printSelf() << "but not handled!";
}

void Worker::onCommand(const Radapter::WorkerMsg &msg)
{
    workerError(this) << ": received Command from: " <<
        msg.sender()->printSelf() << "but not handled!";
}

void Worker::onMsg(const Radapter::WorkerMsg &msg)
{
    workerError(this) << ": received Generic Msg from: " <<
        msg.sender()->printSelf() << "but not handled!";
}

void Worker::onBroadcast(const WorkerMsg &msg)
{
    Q_UNUSED(msg);
}

void Worker::onSendBasic(const JsonDict &msg)
{
    emit sendMsg(prepareMsg(msg));
}

void Worker::onSendRouted(const RoutedObject &obj, const QString &fieldName)
{
    emit sendMsg(prepareMsg(obj.send(fieldName)));
}

void Worker::onWorkerDestroyed(QObject *worker)
{
    d->consumers.remove(qobject_cast<Worker*>(worker));
    d->producers.remove(qobject_cast<Worker*>(worker));
}

void Worker::onMsgFromBroker(const Radapter::WorkerMsg &msg)
{
    if (msg.isBroadcast()) {
        onBroadcast(msg);
        return;
    }
    if (!msg.receivers().contains(this)) return;
    if (isPrintMsgsEnabled()) {
        workerInfo(this, .noquote()) << "<--- TO ###" << msg.printFullDebug();
    }
    if (msg.isReply()) {
        if (!msg.reply()) {
            workerError(this) << "Null Reply, while flagged as reply! Sender: " << msg.sender();
            return;
        }
        if (msg.command() && msg.command()->callback().worker() == this) {
            if (msg.command()->replyOk(msg.reply())) {
                if (msg.command()->callback()) {
                    msg.command()->callback().execute(msg);
                    return;
                }
            } else if (msg.command()->failCallback()) {
                msg.command()->failCallback().execute(msg);
                return;
            } else {
                onReply(msg);
            }
        } else if (msg.command() && msg.command()->replyIgnored()) {
            return;
        } else {
            onReply(msg);
        }
    } else if (msg.isCommand()) {
        if (!msg.command()) {
            workerError(this) << "Null Command, while flagged as command! Sender: " << msg.sender();
            return;
        }
        onCommand(msg);
    } else {
        onMsg(msg);
    }
}

void Worker::onSendMsgPriv(const Radapter::WorkerMsg &msg)
{
    if (!d->wasRun) {
        throw std::runtime_error("Worker sent msg before being run!");
    }
    if (isPrintMsgsEnabled()) {
        workerInfo(this, .noquote()) <<  " <--- FROM ###" << msg.printFullDebug();
    }
}

WorkerProxy* Worker::createPipe(const QList<Interceptor*> &interceptors)
{
    QMutexLocker locker(staticMutex);
    auto proxy = new WorkerProxy(this);
    auto start = new PipeStart(proxy);
    connect(this, &Worker::sendMsg, start, &PipeStart::onSendMsg);
    proxy->setObjectName(workerName());
    d->pipes[proxy].proxy = proxy;
    d->pipes[proxy].start = start;
    for (auto &interceptor : interceptors) {
        if (staticUsedInterceptors->contains(interceptor)) {
            brokerError() << "=======================================================================================";
            brokerError() << "WorkerBase::CreateProxy(): Interceptors list contains copies or already used ones!";
            brokerError() << "WorkerBase::CreateProxy(): This will result in msg loops! Aborting!";
            brokerError() << "=======================================================================================";
            delete proxy;
            throw std::runtime_error("Interceptors list contains copies! Will result in msg loops!");
        }
        interceptor->setParent(nullptr);
        interceptor->moveToThread(proxy->thread());
        interceptor->setParent(proxy);
        staticUsedInterceptors->insert(interceptor);
        connect(interceptor, &QObject::destroyed, [&]() {
            staticUsedInterceptors->remove(interceptor);
        });
    }
    if (interceptors.isEmpty()) {
        // Подключение ОТ брокера (ОТ прок
        connect(start, &PipeStart::msgFromWorker,
                proxy, &WorkerProxy::onMsgFromWorker,
                Qt::DirectConnection);
        return proxy;
    }
    // Подключение К перехватчику (первому/ближнему к воркеру)
    connect(start, &PipeStart::msgFromWorker,
            interceptors[0], &Interceptor::onMsgFromWorker,
            Qt::DirectConnection);
    for (int i = 0; i < interceptors.length() - 1; ++i) {
        // Если перехватчиков более одного, то соединяем их друг с другом
        connect(interceptors[i], &Interceptor::msgFromWorker,
                interceptors[i + 1], &Interceptor::onMsgFromWorker,
                Qt::DirectConnection);
        d->pipes[proxy].interceptors.append(interceptors[i]);
    }
    // Подключение (последний/дальний от воркера) перехватчик --> прокси
    connect(interceptors.last(), &Interceptor::msgFromWorker,
            proxy, &WorkerProxy::onMsgFromWorker,
            Qt::DirectConnection);
    d->pipes[proxy].interceptors.append(interceptors.last());
    connect(proxy, &QObject::destroyed, this, [this, proxy](){
        d->pipes.remove(proxy);
    });
    return proxy;
}

const QString &Worker::workerName() const
{
    return d->config.name;
}

const Worker::WorkerSet &Worker::consumers() const
{
    return d->consumers;
}

const Worker::WorkerSet &Worker::producers() const
{
    return d->producers;
}

QStringList Worker::consumersNames() const
{
    QStringList result;
    for (auto consumer: d->consumers) {
        result.append(consumer->workerName());
    }
    return result;
}

QStringList Worker::producersNames() const
{
    QStringList result;
    for (auto producer: d->producers) {
        result.append(producer->workerName());
    }
    return result;
}

QThread *Worker::workerThread() const
{
    return d->thread;
}

QList<WorkerProxy *> Worker::proxies() const
{
    QList<WorkerProxy *> result;
    for (auto &pipe: d->pipes) {
        result.append(pipe.proxy);
    }
    return result;
}

QList<Interceptor*> Worker::pipe(WorkerProxy *proxy)
{
    if (d->pipes.contains(proxy)) {
        return d->pipes[proxy].interceptors;
    }
    return {};
}


