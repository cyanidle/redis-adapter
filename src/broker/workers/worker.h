#ifndef WORKERBASE_H
#define WORKERBASE_H

#include <QSet>
#include "private/workerdebug.h"
#include "private/workermsg.h"

class RoutedObject;
class QThread;
class JsonDict;
namespace Settings {
class Worker;
}
namespace Radapter {
class Broker;
class WorkerProxy;
class Interceptor;
class WorkerMsg;
class Command;
class Reply;
struct WorkerPrivate;
//! You can override onCommand(cosnt WorkerMsg &) / onReply(cosnt WorkerMsg &) / onMsg(cosnt WorkerMsg &)
class RADAPTER_API Worker : public QObject {
    Q_OBJECT
public:
    using WorkerSet = QSet<Worker*>;
    bool isPrintMsgsEnabled() const;
    bool printEnabled(QtMsgType type) const;
    explicit Worker(const Settings::Worker &settings, QThread *thread);
    WorkerProxy* createPipe(const QList<Interceptor *> &interceptors = {});
    const QString &workerName() const;
    const WorkerSet &consumers() const;
    const WorkerSet &producers() const;
    QStringList consumersNames() const;
    QStringList producersNames() const;
    QThread *workerThread() const;
    QList<WorkerProxy *> proxies() const;
    QList<Interceptor*> pipe(WorkerProxy *proxy);
    Broker *broker() const;
    bool wasStarted() const;
    bool is(const QMetaObject * mobj) const;
    template <typename Target> bool is() const;
    template <typename Target> const Target *as() const;
    template <typename Target> Target *as();
    QString printSelf() const;
    virtual ~Worker();
signals:
    void sendMsg(const Radapter::WorkerMsg &msg);
    void sendBasic(const JsonDict &msg);
    void sendRouted(const RoutedObject &obj, const QString &fieldName = {});
public slots:
    void run();
    void addConsumers(const QStringList &consumers);
    void addProducers(const QStringList &producers);
    void addConsumers(const QSet<Radapter::Worker*> &consumers);
    void addProducers(const QSet<Radapter::Worker*> &producers);
    void addConsumer(Radapter::Worker* consumer, QList<Radapter::Interceptor*> interceptors = {});
    void addProducer(Radapter::Worker* producer, QList<Radapter::Interceptor*> interceptors = {});
protected slots:
    virtual void onRun();
    virtual void onReply(const Radapter::WorkerMsg &msg);
    virtual void onCommand(const Radapter::WorkerMsg &msg);
    virtual void onMsg(const Radapter::WorkerMsg &msg);
    virtual void onBroadcast(const Radapter::WorkerMsg &msg);
private slots:
    void onSendBasic(const JsonDict &msg);
    void onSendRouted(const RoutedObject &obj, const QString &fieldName = {});
    void onWorkerDestroyed(QObject *worker);
    void onSendMsgPriv(const Radapter::WorkerMsg &msg);
    void onMsgFromBroker(const Radapter::WorkerMsg &msg);
protected:
    WorkerMsg prepareMsg(const JsonDict &msg = {}) const;
    WorkerMsg prepareMsg(JsonDict &&msg) const;
    WorkerMsg prepareReply(const WorkerMsg &msg, Reply *reply) const;
    WorkerMsg prepareCommand(Command *command) const;
private:
    WorkerPrivate *d;
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
