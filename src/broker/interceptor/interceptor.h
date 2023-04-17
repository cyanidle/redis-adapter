#ifndef INTERCEPTORBASE_H
#define INTERCEPTORBASE_H

#include "private/global.h"

namespace Radapter {
class RADAPTER_API Interceptor;
class WorkerMsg;
class Worker;
class Broker;
typedef QSet<Interceptor*> Interceptors;
}

//! Класс, перехватывающий трафик
class Radapter::Interceptor : public QObject
{
    Q_OBJECT
public:
    explicit Interceptor();
    const Worker* worker() const;
    QThread *thread();
    virtual Interceptor *newCopy() const = 0;
signals:
    void msgFromWorker(Radapter::WorkerMsg &msg);
public slots:
    virtual void onMsgFromWorker(Radapter::WorkerMsg &msg);
protected:
    Worker* workerNonConst() const;
private:
    friend Worker;
    friend Broker;
};

#endif //INTERCEPTORBASE_H
