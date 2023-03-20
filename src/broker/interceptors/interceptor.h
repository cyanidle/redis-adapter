#ifndef INTERCEPTORBASE_H
#define INTERCEPTORBASE_H

#include "broker/worker/workermsg.h"
#include "private/global.h"

namespace Radapter {
class RADAPTER_API InterceptorBase;
typedef QSet<InterceptorBase*> Interceptors;
}

//! Класс, перехватывающий трафик
class Radapter::InterceptorBase : public QObject
{
    Q_OBJECT
public:
    explicit InterceptorBase();
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
