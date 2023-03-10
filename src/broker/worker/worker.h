#ifndef WORKERBASE_H
#define WORKERBASE_H

#include <QMutexLocker>
#include <QMutex>
#include <QHash>
#include <QThread>
#include "workermsg.h"
#include "workerproxy.h"
#include "broker/interceptors/interceptor.h"
#include "workersettings.h"
#include "broker/events/brokerevent.h"
#include "workerdebug.h"

namespace Radapter {
class Broker;

//! You can override onCommand(cosnt WorkerMsg &) / onReply(cosnt WorkerMsg &) / onMsg(cosnt WorkerMsg &)
class RADAPTER_SHARED_SRC Worker : public QObject {
    Q_OBJECT
public:
    using Set = QSet<Worker*>;
    bool isPrintMsgsEnabled() const;
    bool printEnabled(QtMsgType type) const;
    explicit Worker(const WorkerSettings &settings, QThread *thread);
    //! Фабричный метод, соединяющий объекты в цепь вплоть до прокси, которая является интерфейсом объекта
    /// Interceptor - объект, который находится между прокси и объектом, выполняя некоторую работу над проходящими данными
    WorkerProxy* createProxy(const QSet<InterceptorBase *> &interceptors = {});
    const QString &workerName() const;
    const Set &consumers() const;
    const Set &producers() const;
    QStringList consumersNames() const;
    QStringList producersNames() const;
    QThread *workerThread() const;
    Broker *broker() const;
    bool wasStarted() const;
    template <typename Target> bool is() const;
    template <typename Target> const Target *as() const;
    template <typename Target> Target *as();
    Worker &operator>(Worker &other);
    Worker &operator<(Worker &other);
    QString printSelf() const;
    virtual ~Worker() = default;
signals:
    void fireEvent(const Radapter::BrokerEvent &event);
    void sendMsg(const Radapter::WorkerMsg &msg);
public slots:
    void run();
    void addConsumers(const QStringList &consumers);
    void addProducers(const QStringList &producers);
    void addConsumers(const QSet<Radapter::Worker*> &consumers);
    void addProducers(const QSet<Radapter::Worker*> &producers);
    void addConsumer(Radapter::Worker* consumer);
    void addProducer(Radapter::Worker* producer);
protected slots:
    virtual void onEvent(const Radapter::BrokerEvent &event);
    virtual void onRun();
    virtual void onReply(const Radapter::WorkerMsg &msg);
    virtual void onCommand(const Radapter::WorkerMsg &msg);
    virtual void onMsg(const Radapter::WorkerMsg &msg);
private slots:
    void onWorkerDestroyed(QObject *worker);
    void onSendMsgPriv(const Radapter::WorkerMsg &msg);
    void onMsgFromBroker(const Radapter::WorkerMsg &msg);
    void childDeleted(QObject *who);
protected:
    BrokerEvent prepareEvent(quint32 id, qint16 status, qint16 type, const QVariant &data) const;
    BrokerEvent prepareEvent(quint32 id, qint16 status, qint16 type, QVariant &&data) const;
    BrokerEvent prepareSimpleEvent(quint32 id, qint16 status) const;
    WorkerMsg prepareMsg(const JsonDict &msg = JsonDict()) const;
    WorkerMsg prepareMsg(JsonDict &&msg) const;
    WorkerMsg prepareMsgBad(const QString &reason);
    WorkerMsg prepareReply(const WorkerMsg &msg, Reply *reply) const;
    WorkerMsg prepareCommand(Command *command) const;
    template <typename User, typename...Args>
    WorkerMsg prepareCommand(Command *command, void (User::*cb)(Args...));
    void addInterceptor(InterceptorBase *interceptor);
private:
    template <typename User, typename...Args>
    using MsgCallback = void (User::*)(Args...);
    Set m_consumers;
    Set m_producers;
    QSet<QString> m_consumerNames;
    QSet<QString> m_producerNames;
    WorkerMsg m_baseMsg;
    WorkerProxy* m_proxy;
    QString m_name;
    QThread *m_thread;
    QObject *m_closestConnected{nullptr};
    QList<QMetaObject::Connection> m_closestConnections{};
    QSet<InterceptorBase *> m_InterceptorsToAdd{};
    bool m_wasRun{false};
    bool m_printMsgs{false};

    static QMutex m_mutex;
    static QStringList m_wereCreated;
    static QList<InterceptorBase*> m_usedInterceptors;
    friend Broker;
};

template<typename Target>
bool Worker::is() const {
    static_assert(std::is_base_of<Worker, Target>(), "Must subclass WorkerBase!");
    return metaObject()->inherits(&Target::staticMetaObject);
}

template<typename Target>
Target *Worker::as() {
    return qobject_cast<Target*>(this);
}

template <typename User, typename...Args>
WorkerMsg Worker::prepareCommand(Command *command, MsgCallback<User, Args...> callback) {
    auto cmd = prepareCommand(command);
    if(!is<User>()) {
        throw std::invalid_argument("Must use own method as callback!");
    }
    cmd.setCallback(as<User>(), callback);
    return cmd;
}

template<typename Target>
const Target *Worker::as() const {
    return qobject_cast<const Target*>(this);
}

inline const QString &Worker::workerName() const {
    return m_name;
}

inline const Worker::Set &Worker::consumers() const {
    return m_consumers;
}

inline const Worker::Set &Worker::producers() const {
    return m_producers;
}
}

#endif //WORKERBASE_H
