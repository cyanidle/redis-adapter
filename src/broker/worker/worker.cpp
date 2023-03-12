#include "worker.h"
#include "radapterlogging.h"
#include "broker/broker.h"
#include <QThread>
#include "broker/interceptors/interceptor.h"
#include "workersettings.h"
#include "radapterlogging.h"
#include "workerproxy.h"
#include "broker/events/brokerevent.h"

using namespace Radapter;

QMutex Worker::m_mutex;
QStringList Worker::m_wereCreated = QStringList();
QList<InterceptorBase*> Worker::m_usedInterceptors = {};

Worker::Worker(const WorkerSettings &settings, QThread *thread) :
    QObject(),
    m_consumers(),
    m_producers(),
    m_consumerNames({settings.consumers.begin(), settings.consumers.end()}),
    m_producerNames({settings.producers.begin(), settings.producers.end()}),
    m_baseMsg(this),
    m_proxy(nullptr),
    m_name(settings.name),
    m_thread(thread),
    m_printMsgs(settings.print_msgs)
{
    setObjectName(settings.name);
    if (isPrintMsgsEnabled()) {
        brokerWarn()<< "=== Worker (" << workerName() << "): Running in Debug Mode! ===";
    }
    connect(this, &Worker::sendMsg, &Worker::onSendMsgPriv);
    connect(thread, &QThread::started, this, &Worker::onRun);
    connect(thread, &QThread::destroyed, this, &Worker::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

/// \brief Insert interceptor
/// 1) (If only proxy exists, no interceptors) (Proxy) <--> (Worker)
///     ----->     (Proxy) <--> (NewInterceptor) <--> (Worker)
/// 2) (If interceptor(s) exist) (Proxy) <--> (Interceptor) <--> (Worker)
///     ----->     (Proxy) <--> (Interceptor) <--> (NewInterceptor) <--> (Worker)
void Worker::addInterceptor(InterceptorBase *interceptor)
{
    if (!m_proxy) {
        m_InterceptorsToAdd.insert(interceptor);
        return;
    }
    if (wasStarted()) {
        throw std::runtime_error("Cannot add interceptor during runtime!");
    }
    if (m_usedInterceptors.contains(interceptor)) {
        throw std::runtime_error("Used Interceptor not allowed!");
    }
    auto closestProxy = qobject_cast<WorkerProxy*>(m_closestConnected);
    auto closestInterceptor = qobject_cast<InterceptorBase*>(m_closestConnected);
    if (!closestProxy && !closestInterceptor){
        throw std::runtime_error("Closest connected objecy unknown!");
    }
    for (auto &conn : m_closestConnections) {
        disconnect(conn);
    }
    m_closestConnections.clear();
    m_closestConnections.append(connect(interceptor, &InterceptorBase::msgToWorker, this, &Worker::onMsgFromBroker));
    m_closestConnections.append(connect(this, &Worker::sendMsg, interceptor, &InterceptorBase::onMsgFromWorker));
    connect(interceptor, &InterceptorBase::destroyed, this, &Worker::childDeleted);
    if (closestProxy) {
        connect(interceptor, &InterceptorBase::msgToBroker, closestProxy, &WorkerProxy::onMsgFromWorker);
        connect(closestProxy, &WorkerProxy::msgToWorker, interceptor, &InterceptorBase::onMsgFromBroker);
    } else if (closestInterceptor) {
        connect(interceptor, &InterceptorBase::msgToBroker, closestInterceptor, &InterceptorBase::onMsgFromWorker);
        connect(closestInterceptor, &InterceptorBase::msgToWorker, interceptor, &InterceptorBase::onMsgFromBroker);
    } else {
        throw std::runtime_error("Unknown Error while adding interceptor!");
    }
}

bool Worker::isPrintMsgsEnabled() const
{
    return m_printMsgs;
}

bool Worker::printEnabled(QtMsgType type) const
{
    return broker()->isDebugEnabled(workerName(), type);
}

Broker *Worker::broker() const
{
    return Broker::instance();
}

bool Worker::wasStarted() const
{
    return m_wasRun;
}

Worker &Worker::operator>(Worker &other)
{
    addConsumers({&other});
    return *this;
}

Worker &Worker::operator<(Worker &other)
{
    addProducers({&other});
    return *this;
}

QString Worker::printSelf() const
{
    return QStringLiteral("[%2 (%1)]: ").arg(metaObject()->className(), workerName());
}

void Worker::run()
{
    if (m_wasRun) throw std::runtime_error("WorkerBase::onRun() called multiple times for: " + printSelf().toStdString());
    moveToThread(workerThread());
    for (auto &producerName : qAsConst(m_producerNames)) {
        auto worker = broker()->getWorker(producerName);
        if (!worker) throw std::runtime_error("Nonexistent required worker: " + producerName.toStdString());
        addProducers({worker});
    }
    for (auto &consumerName : qAsConst(m_consumerNames)) {
        auto worker = broker()->getWorker(consumerName);
        if (!worker) throw std::runtime_error("Nonexistent required worker: " + consumerName.toStdString());
        addConsumers({worker});
    }
    workerThread()->start();
    m_wasRun = true;
}

void Worker::onRun()
{
    workerInfo(this) << ": started!";
}

void Worker::addConsumers(const QStringList &consumers)
{
    for (auto &name : consumers) {
        m_consumerNames.insert(name);
        if (wasStarted()) {
            auto worker = broker()->getWorker(name);
            if (!worker) throw std::runtime_error("Nonexistent worker: " + name.toStdString());
            addConsumer(worker);
        }
    }
}

void Worker::addProducers(const QStringList &producers)
{
    for (auto &name : producers) {
        m_producerNames.insert(name);
        if (wasStarted()) {
            auto worker = broker()->getWorker(name);
            if (!worker) throw std::runtime_error("Nonexistent worker: " + name.toStdString());
            addProducer(worker);
        }
    }
}

void Worker::addConsumers(const Set &consumers)
{
    for (auto &worker : consumers) {
        addConsumer(worker);
    }
}

void Worker::addProducers(const Set &producers)
{
    for (auto &worker : producers) {
        addProducer(worker);
    }
}

void Worker::addConsumer(Worker *consumer)
{
    m_consumers.insert(consumer);
    if (!consumer->producers().contains(this)) {
        consumer->addProducer(this);
    }
    connect(consumer, &QObject::destroyed, this, &Worker::onWorkerDestroyed);
    if (!m_consumerNames.contains(consumer->workerName())) {
        m_consumerNames.insert(consumer->workerName());
    }
    m_baseMsg.m_receivers.insert(consumer);
}

void Worker::addProducer(Worker *producer)
{
    m_producers.insert(producer);
    if (!producer->consumers().contains(this)) {
        producer->addConsumer(this);
    }
    connect(producer, &QObject::destroyed, this, &Worker::onWorkerDestroyed);
    if (!m_producerNames.contains(producer->workerName())) {
        m_producerNames.insert(producer->workerName());
    }
}

void Worker::onEvent(const BrokerEvent &event)
{
    Q_UNUSED(event);
}

WorkerMsg Worker::prepareMsgBad(const QString &reason)
{
    auto msg = m_baseMsg;
    msg.updateId();
    msg.serviceData(WorkerMsg::ServiceBadReasonField) = reason.isEmpty() ? "Not given" : reason;
    msg.setFlag(WorkerMsg::MsgBad);
    return msg;
}

WorkerMsg Worker::prepareMsg(const JsonDict &msg) const
{
    auto wrapped = m_baseMsg;
    wrapped.updateId();
    wrapped.setJson(msg);
    return wrapped;
}

WorkerMsg Worker::prepareMsg(JsonDict &&msg) const
{
    auto wrapped = m_baseMsg;
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
    auto wrapped = m_baseMsg;
    wrapped.setCommand(command);
    wrapped.updateId();
    return wrapped;
}

void Worker::onReply(const Radapter::WorkerMsg &msg)
{
    if (msg.command()->isReplyExpected()) {
        workerError(this) << ": received Reply from: " <<
            msg.sender()->printSelf() << "but not handled!";
        throw std::runtime_error("Unhandled reply!");
    }
}

void Worker::onCommand(const Radapter::WorkerMsg &msg)
{
    workerError(this) << ": received Command from: " <<
        msg.sender()->printSelf() << "but not handled!";
    throw std::runtime_error("Unhandled command!");
}

void Worker::onMsg(const Radapter::WorkerMsg &msg)
{
    workerError(this) << ": received Generic Msg from: " <<
        msg.sender()->printSelf() << "but not handled!";
    throw std::runtime_error("Unhandled msg!");
}

void Worker::onWorkerDestroyed(QObject *worker)
{
    m_consumers.remove(qobject_cast<Worker*>(worker));
    m_producers.remove(qobject_cast<Worker*>(worker));
}

void Worker::onMsgFromBroker(const Radapter::WorkerMsg &msg)
{
    if (isPrintMsgsEnabled()) {
        workerInfo(this, .noquote()) << "<--- TO ###" << msg.printFullDebug();
    }
    if (msg.isReply()) {
        if (!msg.reply()) {
            workerError(this) << "Null Reply, while flagged as reply! Sender: " << msg.sender();
            return;
        }
        if (msg.command() && msg.command()->callback() && msg.command()->callback()->worker() == this) {
            msg.command()->callback()->execute(msg);
        } else {
            onReply(msg);
        }
    }
    else if (msg.isCommand()) {
        if (!msg.command()) {
            workerError(this) << "Null Command, while flagged as command! Sender: " << msg.sender();
            return;
        }
        onCommand(msg);
    }
    else onMsg(msg);
}

void Worker::childDeleted(QObject *who)
{
    workerWarn(this) << "Deleting...; Reason --> Important Child died: " << who;
    deleteLater();
}

BrokerEvent Worker::prepareEvent(quint32 id, qint16 status, qint16 type, const QVariant &data) const
{
    return {const_cast<Worker*>(this), id, status, type, data};
}

BrokerEvent Worker::prepareEvent(quint32 id, qint16 status, qint16 type, QVariant &&data) const
{
    return {const_cast<Worker*>(this), id, status, type, std::move(data)};
}

BrokerEvent Worker::prepareSimpleEvent(quint32 id, qint16 status) const
{
    return {const_cast<Worker*>(this), id, status, 0, {}};
}

void Worker::onSendMsgPriv(const Radapter::WorkerMsg &msg)
{
    if (!m_wasRun) {
        throw std::runtime_error("Worker sent msg before being run!");
    }
    if (isPrintMsgsEnabled()) {
        workerInfo(this, .noquote()) <<  " <--- FROM ###" << msg.printFullDebug();
    }
}

WorkerProxy* Worker::createProxy(const QSet<InterceptorBase*> &interceptors)
{
    QMutexLocker locker(&m_mutex);
    if (m_proxy == nullptr) {
        m_proxy = new WorkerProxy();
        m_proxy->setObjectName(workerName());
        m_proxy->setParent(this);
        auto filtered = QList<InterceptorBase*>();
        for (auto &interceptor : interceptors) {
            if (!filtered.contains(interceptor) || !m_usedInterceptors.contains(interceptor)) {
                interceptor->setParent(this);
                filtered.append(interceptor);
                m_usedInterceptors.append(interceptor);
            } else {
                brokerError() << "=======================================================================================";
                brokerError() << "WorkerBase::CreateProxy(): Interceptors list contains copies or already used ones!";
                brokerError() << "WorkerBase::CreateProxy(): This will result in msg loops! Aborting!";
                brokerError() << "=======================================================================================";
                delete m_proxy;
                m_proxy = nullptr;
                throw std::runtime_error("Interceptors list contains copies! Will result in msg loops!");
            }
        }
        if (filtered.isEmpty()) {
            // Если удаляется любой из перехватчиков или прокси, воркер следует за ними
            connect(m_proxy, &WorkerProxy::destroyed,
                    this, &Worker::childDeleted,
                    Qt::DirectConnection);
            // Подключение ОТ брокера (ОТ прок
            auto outConn = connect(this, &Worker::sendMsg,
                    m_proxy, &WorkerProxy::onMsgFromWorker,
                    Qt::DirectConnection);
            // Подключение ОТ брокера (ОТ прокси)
            auto inConn = connect(m_proxy, &WorkerProxy::msgToWorker,
                    this, &Worker::onMsgFromBroker,
                    Qt::DirectConnection);
            m_closestConnected = m_proxy;
            m_closestConnections = {outConn, inConn};
        } else {
            connect(filtered[0], &InterceptorBase::destroyed,
                    this, &Worker::childDeleted,
                    Qt::DirectConnection);
            // Подключение К перехватчику (первому/ближнему к воркеру)
            auto outConn = connect(this, &Worker::sendMsg,
                    filtered[0], &InterceptorBase::onMsgFromWorker,
                    Qt::DirectConnection);
            // Подключение ОТ перехватчика (первого)
            auto inConn = connect(filtered[0], &InterceptorBase::msgToWorker,
                    this, &Worker::onMsgFromBroker,
                    Qt::DirectConnection);
            m_closestConnected = filtered[0];
            m_closestConnections = {outConn, inConn};
            for (int i = 0; i < filtered.length() - 1; ++i) {
                connect(filtered[i], &InterceptorBase::destroyed,
                        this, &Worker::childDeleted,
                        Qt::DirectConnection);
                // Если перехватчиков более одного, то соединяем их друг с другом
                connect(filtered[i], &InterceptorBase::msgToBroker,
                        filtered[i + 1], &InterceptorBase::onMsgFromWorker,
                        Qt::DirectConnection);
                connect(filtered[i + 1], &InterceptorBase::msgToWorker,
                        filtered[i], &InterceptorBase::onMsgFromBroker,
                        Qt::DirectConnection);
            }
            connect(filtered.last(), &InterceptorBase::destroyed,
                    this, &Worker::childDeleted,
                    Qt::DirectConnection);
            // Подключение (последний/дальний от воркера) перехватчик --> прокси
            connect(filtered.last(), &InterceptorBase::msgToBroker,
                    m_proxy, &WorkerProxy::onMsgFromWorker,
                    Qt::DirectConnection);
            // Подключение прокси --> (последний) перехватчик
            connect(m_proxy, &WorkerProxy::msgToWorker,
                    filtered.last(), &InterceptorBase::onMsgFromBroker,
                    Qt::DirectConnection);
        }
    }
    for (auto &inter : qAsConst(m_InterceptorsToAdd)) {
        addInterceptor(inter);
    }
    return m_proxy;
}

QStringList Worker::consumersNames() const
{
    return {m_consumerNames.begin(), m_consumerNames.end()};
}

QStringList Worker::producersNames() const
{
    return {m_producerNames.begin(), m_producerNames.end()};
}

QThread *Worker::workerThread() const
{
    return m_thread;
}


