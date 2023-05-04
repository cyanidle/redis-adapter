#ifndef WORKERBASE_H
#define WORKERBASE_H

#include "private/global.h"
#include "private/workerdebug.h"
#include "private/workermsg.h"
class QThread;
template<class T>
class QSet;
class JsonDict;
namespace Serializable {struct IsFieldCheck;}
namespace Settings {struct Worker;}
namespace State{struct Json;}
namespace Radapter {
class Broker;
class WorkerProxy;
class Interceptor;
class WorkerMsg;
class Command;
class Reply;
//! You can override onCommand(cosnt WorkerMsg &) / onReply(cosnt WorkerMsg &) / onMsg(cosnt WorkerMsg &)
class RADAPTER_API Worker : public QObject {
    Q_OBJECT
    struct Private;
public:
    enum RoleFlags {
        Consumer = 1 << 1,
        Producer = 1 << 2,
        ConsumerProducer = Consumer | Producer
    };
    Q_ENUM(RoleFlags)
    Q_DECLARE_FLAGS(Role, RoleFlags)
    using WorkerSet = QSet<Worker*>;
    explicit Worker(const Settings::Worker &settings, QThread *thread);
    bool isPrintMsgsEnabled() const;
    bool printEnabled(QtMsgType type) const;
    const QString &workerName() const;
    const WorkerSet &consumers() const;
    const WorkerSet &producers() const;
    QStringList consumersNames() const;
    QStringList producersNames() const;
    QThread *workerThread() const;
    QList<WorkerProxy *> proxies() const;
    QList<Interceptor*> pipe(WorkerProxy *proxy) const;
    Broker *broker() const;
    bool wasStarted() const;
    bool is(const QMetaObject * mobj) const;
    template <typename Target> bool is() const;
    template <typename Target> const Target *as() const;
    template <typename Target> Target *as();
    void addConsumer(Radapter::Worker* consumer, const QList<Radapter::Interceptor*> &interceptors = {});
    void addProducer(Radapter::Worker* producer, const QList<Radapter::Interceptor*> &interceptors = {});
    Role getRole() const;
    QString printSelf() const;
    virtual ~Worker();
signals:
    void sendMsg(const Radapter::WorkerMsg &msg);
    void send(const JsonDict &msg);
    void sendKey(const QString &key, const QVariant &value);
    void sendState(const State::Json &obj);
    void sendStatePart(const State::Json &obj, const Serializable::IsFieldCheck &field);

    void connectedToConsumer(Radapter::Worker *consumer, QPrivateSignal);
    void connectedToProducer(Radapter::Worker *producer, QPrivateSignal);
public slots:
    void run();
protected slots:
    virtual void onRun();
    virtual void onReply(const Radapter::WorkerMsg &msg);
    virtual void onCommand(const Radapter::WorkerMsg &msg);
    virtual void onMsg(const Radapter::WorkerMsg &msg);
    virtual void onBroadcast(const Radapter::WorkerMsg &msg);
private slots:
    void onWorkerDestroyed(QObject *worker);
    void onSendMsgPriv(const Radapter::WorkerMsg &msg);
    void onMsgFromBroker(const Radapter::WorkerMsg &msg);
protected:
    void setRole(Role role);
    Settings::Worker &workerConfig();
    WorkerMsg prepareMsg(const JsonDict &msg = {}) const;
    WorkerMsg prepareMsg(JsonDict &&msg) const;
    WorkerMsg prepareReply(const WorkerMsg &msg, Reply *reply) const;
    WorkerMsg prepareCommand(Command *command) const;
private:
    WorkerProxy* createPipe(const QList<Interceptor *> &interceptors = {});

    Private *d;
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

template<typename Target>
const Target *Worker::as() const {
    return qobject_cast<const Target*>(this);
}

}

#endif //WORKERBASE_H
