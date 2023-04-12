#ifndef INTERCEPTORBASE_H
#define INTERCEPTORBASE_H

#include "private/global.h"

namespace Radapter {
class RADAPTER_API Interceptor;
class WorkerMsg;
class Worker;
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
signals:
    void msgToWorker(const Radapter::WorkerMsg &msg);
    void msgToBroker(const Radapter::WorkerMsg &msg);

public slots:
    virtual void onMsgFromWorker(const Radapter::WorkerMsg &msg);
    virtual void onMsgFromBroker(const Radapter::WorkerMsg &msg);
protected:
    Worker* workerNonConst() const;
public slots:

private:
};

#endif //INTERCEPTORBASE_H
